/**
 * @file OpenGLBackend.ixx
 * @brief OpenGL backend for render-pass execution and indexed draw submission.
 */
module;

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <cassert>
#include <utility>
#include <type_traits>
#include "helios-opengl-config.h"


export module helios.opengl.OpenGLBackend;

import helios.ecs.types;

import helios.math;

import helios.engine.util.log;

import helios.engine.rendering.common.types;
import helios.engine.rendering.common.components;

import helios.engine.spatial.components;

import helios.engine.core.components;

import helios.opengl.components;


import helios.engine.rendering.mesh;
import helios.engine.rendering.shader;
import helios.engine.rendering.material;
import helios.engine.rendering.framebuffer;
import helios.engine.rendering.viewport;
import helios.engine.util.Colors;

import helios.engine.scene.types.SceneMemberRenderContext;

import helios.engine.runtime.world.EngineWorld;;

using namespace helios::engine::core::components;
using namespace helios::engine::rendering;
using namespace helios::engine::rendering::mesh::components;
using namespace helios::engine::rendering::shader;
using namespace helios::engine::rendering::shader::types;
using namespace helios::opengl;
using namespace helios::opengl::components;
using namespace helios::engine::spatial::components;
using namespace helios::engine::rendering::material::types;
using namespace helios::engine::rendering::common::types;
using namespace helios::engine::rendering::common::components;
using namespace helios::engine::rendering::mesh::types;
using namespace helios::engine::rendering::framebuffer::types;
using namespace helios::engine::rendering::viewport::types;
using namespace helios::engine::scene::types;
using namespace helios::engine::runtime::world;
using namespace helios::engine::util::log;
using namespace helios::ecs::types;

#define HELIOS_LOG_SCOPE "helios::opengl"
export namespace helios::opengl {


    /**
     * @brief Applies render-pass state and executes OpenGL draw calls.
     *
     * @details
     * `OpenGLBackend` is intentionally thin and stateful: it references existing
     * worlds and translates ECS render data into OpenGL state changes.
     */
    class OpenGLBackend {
    private:

        /**
         * @brief Cached uniform values grouped by uniform update scope.
         * @tparam TUniformScope Scope tag that separates pass- and draw-level data.
         */
        template<typename TUniformScope>
        struct UniformValueBag {
            /** @brief Model/world matrix used for per-draw updates. */
            helios::math::mat4f worldMatrix{};
            /** @brief View matrix used for pass-level updates. */
            helios::math::mat4f viewMatrix{};
            /** @brief Projection matrix used for pass-level updates. */
            helios::math::mat4f projectionMatrix{};
            /** @brief Optional color payload for future semantic mappings. */
            helios::math::vec4f color{};
        };

        /** @brief Render-resource ECS world (meshes, shaders, materials). */
        RenderResourceWorld& renderResourcesWorld_;

        /** @brief Render-target ECS world (framebuffers, viewports). */
        RenderTargetWorld& renderTargetsWorld_;

        /** @brief Tracks whether GL function pointers were initialized. */
        bool isInitialized_ = false;

        inline static const helios::engine::util::log::Logger& logger_ = helios::engine::util::log::LogManager::loggerForScope(
            HELIOS_LOG_SCOPE
        );

        /** @brief Currently active viewport for pass consistency checks. */
        ViewportHandle currentViewportHandle_{};
        /** @brief Currently bound shader to avoid redundant `glUseProgram` calls. */
        ShaderHandle currentShaderHandle_{};

        /** @brief Cached pass-scope uniform values (view/projection). */
        UniformValueBag<UniformScope::Pass> passUniformValueBag_{};
        /** @brief Cached draw-scope uniform values (model/world and per-draw data). */
        UniformValueBag<UniformScope::Draw> drawUniformValueBag_{};

    public:



        /**
         * @brief Constructs the backend from render resource and target worlds.
         *
         * @param renderResourcesWorld Render resource world.
         * @param renderTargetsWorld Render target world.
         */
        explicit OpenGLBackend(
            RenderResourceWorld& renderResourcesWorld,
            RenderTargetWorld& renderTargetsWorld
        ) :
        renderResourcesWorld_(renderResourcesWorld),
        renderTargetsWorld_(renderTargetsWorld)
        {}


        /**
         * @brief Begins a render pass and configures framebuffer, viewport, and clear state.
         *
         * @details
         * In debug builds, missing entities and invalid framebuffer handles are
         * reported and asserted. The clear mask is derived from `ClearFlags`.
         *
         * @param renderPassContext Render pass target context.
         */
        void beginRenderPass(const RenderPassContext& renderPassContext) {

            auto viewportHandle = renderPassContext.viewportHandle;

            currentViewportHandle_ = viewportHandle;
            passUniformValueBag_.projectionMatrix = renderPassContext.projectionMatrix;
            passUniformValueBag_.viewMatrix       = renderPassContext.viewMatrix;

            auto framebufferHandle = renderPassContext.framebufferHandle;

            auto viewport = renderTargetsWorld_.findEntity<ViewportHandle>(viewportHandle);
            auto framebuffer = renderTargetsWorld_.findEntity<FramebufferHandle>(framebufferHandle);

            #ifdef HELIOS_DEBUG
            if (!viewport) {
                logger_.error("Missing Viewport for handle {0}.", viewportHandle.entityId);
                assert(viewport && "Missing Viewport for handle.");
            }
            if (!framebuffer) {
                logger_.error("Missing Framebuffer for handle {0}.", framebufferHandle.entityId);
                assert(framebuffer && "Missing Framebuffer for handle.");
            }
            #endif

            const auto clearColor = framebuffer->get<ColorComponent<FramebufferHandle>>()->value();
            const auto clearFlags = std::to_underlying(framebuffer->get<ClearComponent<FramebufferHandle>>()->flags);
            const auto framebufferId = framebuffer->get<OpenGLFramebufferIdComponent<FramebufferHandle>>()->value();

            auto viewportBounds  = viewport->get<BoundsComponent<ViewportHandle>>()->value();
            auto framebufferSize = framebuffer->get<Size2DComponent<FramebufferHandle>>()->value();

            glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

            #ifdef HELIOS_DEBUG
            const auto isValidFramebuffer = framebufferId == 0 ||
                (glIsFramebuffer(framebufferId) == GL_TRUE && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            if (!isValidFramebuffer) {
                logger_.error("Framebuffer with id {0} undefined.", framebufferId);
                assert(isValidFramebuffer && "Framebuffer id does not seem to be a valid id.");
            }
            #endif


            // this is equally important for the GlpyhTextRenderer
            // enable blending since the font's fragment shader uses the alpha channel
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

            const auto x = static_cast<int>(framebufferSize[0] * viewportBounds[0]);
            const auto y = static_cast<int>(framebufferSize[1] * viewportBounds[1]);
            const auto width = static_cast<int>(framebufferSize[0] * viewportBounds[2]);
            const auto height = static_cast<int>(framebufferSize[1] * viewportBounds[3]);

            const auto clearMask = ((clearFlags & std::to_underlying(ClearFlags::Color)) ? GL_COLOR_BUFFER_BIT : 0) |
                ((clearFlags & std::to_underlying(ClearFlags::Depth)) ? GL_DEPTH_BUFFER_BIT : 0) |
                ((clearFlags & std::to_underlying(ClearFlags::Stencil)) ? GL_STENCIL_BUFFER_BIT : 0);

            glViewport(x, y, width, height);
            glScissor(x, y, width, height);
            glEnable(GL_SCISSOR_TEST);
            if (clearMask != 0) {
                glClear(clearMask);
            }


        }

        /**
         * @brief Renders one extracted scene member.
         *
         * @details
         * Resolves shader and mesh entities from render-resource handles, validates
         * expected OpenGL components in debug/error paths, binds program and VAO,
         * and submits one indexed `glDrawElements` call.
         *
         * @tparam THandle Scene member handle type.
         * @param renderContext Per-member render context.
         */
        template<typename THandle>
        void doRender(const SceneMemberRenderContext<THandle>& renderContext)  {

            assert(renderContext.viewportHandle == currentViewportHandle_ && "ViewportHandle out of sync");

            auto meshHandle = renderContext.meshHandle;
            auto shaderHandle = renderContext.shaderHandle;

            // shader
            auto shaderEntity = renderResourcesWorld_.findEntity(shaderHandle);
            if (!shaderEntity) {
                logger_.error("ShaderEntity expected, but not found");
                assert(false && "ShaderEntity not found");
                return;
            }
            using ShaderHandle_t = std::remove_cvref_t<decltype(shaderHandle)>;

            auto* openglShader = shaderEntity->template get<OpenGLShaderComponent<ShaderHandle>>();
            if (!openglShader) {
                logger_.error("OpenGLShader expected, but not found");
                assert(false && "OpenGLShader not found");
                return;
            }

            // opengl mesh
            auto meshEntity = renderResourcesWorld_.findEntity(meshHandle);
            if (!meshEntity) {
                logger_.error("MeshEntity expected, but not found");
                assert(false && "MeshEntity not found");
                return;
            }
            using MeshHandle_t = std::remove_cvref_t<decltype(meshHandle)>;

            auto* openglMesh = meshEntity->template get<OpenGLMeshComponent<MeshHandle_t>>();
            if (!openglMesh) {
                logger_.error("OpenGLMesh expected, but not found");
                assert(false && "OpenGLMesh not found");
                return;
            }

            if (currentShaderHandle_ != shaderHandle) {
                glUseProgram(openglShader->programId);

                updateUniformValues<UniformScope::Pass>(*shaderEntity, passUniformValueBag_);
                currentShaderHandle_ = shaderHandle;
            }

            unsigned int vao = openglMesh->vao;
            glBindVertexArray(vao);

            drawUniformValueBag_.worldMatrix = renderContext.worldMatrix;
            updateUniformValues<UniformScope::Draw>(*shaderEntity, drawUniformValueBag_);

            if (passUniformValueBag_.viewMatrix(0, 0) != 1.0f) {
                logger_.debug("YO!");
            }

            glDrawElements(
                openglMesh->primitiveType,
                openglMesh->indexCount,
                GL_UNSIGNED_INT,
                nullptr
            );

        }

        /**
         * @brief Uploads cached uniform values for a specific uniform scope.
         *
         * @details Resolves `OpenGLUniformLocationComponent<ShaderHandle, TUniformScope>`
         * on the shader entity and writes matching values from `uniformValueBag`
         * to all cached OpenGL uniform locations. Locations with `-1` are skipped.
         *
         * @tparam TUniformScope Uniform lifetime scope (for example pass or draw).
         * @param shaderEntity Shader entity holding location cache and shader data.
         * @param uniformValueBag Source values to upload for this scope.
         */
        template<typename TUniformScope>
        void updateUniformValues(ShaderEntity shaderEntity, UniformValueBag<TUniformScope>& uniformValueBag) {

            auto* ulc = shaderEntity.get<OpenGLUniformLocationComponent<ShaderHandle, TUniformScope>>();

            if (!ulc) {
                logger_.warn("No OpenGLUniformLocationComponent for scope {0}, skipping uniform updates.", typeid(TUniformScope).name());
            } else {

                for (std::size_t i = 0; i < ulc->locations.size(); ++i) {
                    GLint location = ulc->locations[i];
                    if (location == -1) {
                        continue;
                    }
                    switch (static_cast<UniformSemantics>(i)) {
                    case UniformSemantics::ViewMatrix:
                        glUniformMatrix4fv(location, 1, GL_FALSE, helios::math::value_ptr(uniformValueBag.viewMatrix));
                        break;
                    case UniformSemantics::ProjectionMatrix:
                        glUniformMatrix4fv(location, 1, GL_FALSE, helios::math::value_ptr(uniformValueBag.projectionMatrix));
                        break;
                    case UniformSemantics::ModelMatrix:
                        glUniformMatrix4fv(location, 1, GL_FALSE, helios::math::value_ptr(uniformValueBag.worldMatrix));
                        break;

                    }
                }

            }


        }

        /**
         * @brief Ends the current render pass.
         *
         * @details
         * Restores transient OpenGL state enabled in `beginRenderPass`.
         *
         * @param renderPassContext Render pass target context.
         */
        void endRenderPass(const RenderPassContext& renderPassContext) {

            glDisable(GL_SCISSOR_TEST);
            glBindVertexArray(0);

            // new ViewportHandle -> invalid
            currentViewportHandle_ = ViewportHandle{};
            currentShaderHandle_   = ShaderHandle{};
            passUniformValueBag_   = UniformValueBag<UniformScope::Pass>{};
            drawUniformValueBag_   = UniformValueBag<UniformScope::Draw>{};
        }

        /**
         * @brief Applies window hints for an OpenGL core-profile context.
         *
         * @details
         * The backend currently requests OpenGL 4.1 core profile for macOS
         * compatibility.
         */
        void provideWindowHints() noexcept {

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        }

        /**
         * @brief Initializes OpenGL function pointers through GLFW.
         *
         * @pre A valid, current OpenGL context exists on the calling thread.
         * @post `isInitialized()` returns `true` on success.
         * @return `true` if loading succeeded, otherwise `false`.
         */
        bool init() {

            assert(!isInitialized_ && "Backend already initialized");

            const GLADloadfunc procAddressLoader = glfwGetProcAddress;
            const int gl_ver = gladLoadGL(procAddressLoader);

            if (gl_ver == 0) {
                logger_.error("Failed to load OpenGL");
                assert(false && "Failed to load OpenGL");
                return false;
            }

            logger_.info("OpenGL {0}.{1} loaded", GLAD_VERSION_MAJOR(gl_ver), GLAD_VERSION_MINOR(gl_ver));

            isInitialized_ = true;
            return true;

        }

        /**
         * @brief Reports whether OpenGL function loading completed successfully.
         */
        bool isInitialized() {
            return isInitialized_;
        }




    };
} // namespace helios::opengl
