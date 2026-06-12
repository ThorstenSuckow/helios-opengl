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
#include <span>
#include "helios-opengl-config.h"
#include <optional>

export module helios.opengl.OpenGLBackend;

import helios.math;

import helios.engine.util.log;

import helios.engine.rendering.viewport.ViewportEntity;

import helios.engine.rendering.renderTarget.RenderTargetEntity;
import helios.engine.rendering.common.types;
import helios.engine.rendering.common.components;

import helios.engine.spatial.components;

import helios.engine.core.components;

import helios.opengl.OpenGLUniformWriter;
import helios.opengl.components;
import helios.opengl.types;


import helios.engine.rendering.mesh;
import helios.engine.rendering.shader;
import helios.engine.rendering.material;
import helios.engine.rendering.renderTarget;
import helios.engine.rendering.viewport;
import helios.engine.util.Colors;

import helios.engine.scene.components;
import helios.engine.scene.types;

import helios.engine.runtime.world.EngineWorld;

import helios.opengl.types;

using namespace helios::engine::core::components;
using namespace helios::engine::rendering;
using namespace helios::engine::rendering::mesh::components;
using namespace helios::engine::rendering::shader;
using namespace helios::engine::rendering::shader::types;
using namespace helios::opengl;
using namespace helios::opengl::components;
using namespace helios::opengl::types;
using namespace helios::engine::spatial::components;
using namespace helios::engine::rendering::material::types;
using namespace helios::engine::rendering::common::types;
using namespace helios::engine::rendering::common::components;
using namespace helios::engine::rendering::shader::components;
using namespace helios::engine::rendering::mesh::types;
using namespace helios::engine::rendering::renderTarget;
using namespace helios::engine::rendering::renderTarget::types;
using namespace helios::engine::rendering::viewport;
using namespace helios::engine::rendering::viewport::types;
using namespace helios::engine::scene::components;
using namespace helios::engine::scene::types;
using namespace helios::engine::runtime::world;
using namespace helios::engine::util::log;

#define HELIOS_LOG_SCOPE "helios::opengl"
export namespace helios::opengl {


    /**
     * @brief Applies render-pass state and executes OpenGL draw calls.
     *
     * @details `OpenGLBackend` is intentionally thin and stateful: it references existing
     * worlds and translates ECS render data into OpenGL state changes.
     */
    class OpenGLBackend {
    private:

        /**
         * @brief Tracks whether GL function pointers were initialized.
         */
        bool isInitialized_ = false;

        /**
         * @brief Scoped logger used for backend diagnostics.
         */
        inline static const helios::engine::util::log::Logger& logger_ = helios::engine::util::log::LogManager::loggerForScope(
            HELIOS_LOG_SCOPE
        );

        /**
         * @brief Cached pointer to the currently bound OpenGL mesh component.
         */
        OpenGLMeshComponent<MeshHandle>* currentOpenGLMesh_ = nullptr;

        /**
         * @brief Cached pass-scope uniforms (typically view/projection).
         */
        UniformValueBag<UniformScope::Pass> passUniformValueBag_{};

        /**
         * @brief Cached draw-scope uniforms (for example model matrix/material color).
         */
        UniformValueBag<UniformScope::Draw> drawUniformValueBag_{};


        /**
         * @brief Currently active render target for nested viewport processing.
         */
        RenderTargetHandle currentRenderTargetHandle_{};

        /**
         * @brief Currently bound shader to avoid redundant `glUseProgram` calls.
         */
        ShaderHandle currentShaderHandle_{};


        /**
         * @brief Engine world used to resolve render entities and components.
         */
        EngineWorld& engineWorld_;

        /**
         * @brief Per-viewport camera matrices used for a render pass.
         */
        struct ViewProjection {
            helios::math::mat4f viewMatrix;
            helios::math::mat4f projectionMatrix;
        };

        /**
         * @brief Resolves view and projection matrices for a viewport's bound camera.
         *
         * @param vieportEntity Viewport entity used to resolve camera bindings.
         * @return View/projection pair on success, otherwise `std::nullopt`.
         */
        [[nodiscard]] std::optional<ViewProjection> viewProjection(const ViewportEntity& vieportEntity) const noexcept {

            auto* cbc = vieportEntity.get<CameraBindingComponent<ViewportHandle>>();
            if (!cbc) {
                logger_.error("Expected CameraBindingComponent on ViewportEntity, but couldn't find any.");
                return std::nullopt;
            }
            auto camera = engineWorld_.find(cbc->targetHandle());
            if (!camera) {
                logger_.error("Expected CameraEntity, but couldn't find any.");
                return std::nullopt;
            }
            using CameraHandleType = std::remove_cvref_t<decltype(cbc->targetHandle())>;
            auto* vm = camera->get<ViewMatrixComponent<CameraHandleType>>();
            if (!vm) {
                logger_.error("Expected ViewMatrixComponent, but couldn't find any.");
                return std::nullopt;
            }

            auto* pm = camera->get<ProjectionMatrixComponent<CameraHandleType>>();
            if (!pm) {
                logger_.error("Expected ProjectionMatrixComponent, but couldn't find any.");
                return std::nullopt;
            }

            return ViewProjection{
                vm->value(), pm->value()
            };

        }

        /**
         * @brief Uploads cached uniform values for a specific uniform scope.
         *
         * @details Resolves `OpenGLUniformWriteOperationsComponent<ShaderHandle, TUniformScope>`
         * on the shader entity and forwards its operation list plus values from
         * `UniformValueBag` to `OpenGLUniformWriter`. If no write-plan component
         * exists, the method logs an error and asserts in debug builds.
         *
         * @tparam TUniformScope Uniform lifetime scope (for example pass or draw).
         * @param shaderEntity Shader entity holding location cache and shader data.
         * @param uniformValueBag Source values to upload for this scope.
         */
        template<typename TUniformScope>
        void writeUniformValues(ShaderEntity shaderEntity, UniformValueBag<TUniformScope>& uniformValueBag) noexcept {

            auto* ulc = shaderEntity.get<OpenGLUniformWriteOperationsComponent<ShaderHandle, TUniformScope>>();

            if (!ulc) {
                logger_.error("OpenGLUniformWriteOperationsComponent<{0}> expected, but not found", typeid(TUniformScope).name());
                assert(false && "OpenGLUniformWriteOperationsComponent not found");
                return;
            }

            OpenGLUniformWriter::write(ulc->operations, uniformValueBag);
        }


        /**
         * @brief Applies clear color and clear mask based on optional components.
         *
         * @tparam THandle Handle type used for component lookup.
         * @tparam TEntity Entity wrapper type exposing `get<...>()`.
         * @param entity Entity to query for `ColorComponent` and `ClearComponent`.
         */
        template<typename THandle, typename TEntity>
        void clearColor(TEntity& entity) noexcept {

            auto* colorComp = entity->template get<ColorComponent<THandle>>();
            auto* clearComp = entity->template get<ClearComponent<THandle>>();

            if (colorComp) {
                const auto clearColor = colorComp->value();
                glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
            }

            if (clearComp) {
                const auto clearFlags = std::to_underlying(clearComp->flags);
                const auto clearMask = ((clearFlags & std::to_underlying(ClearFlags::Color)) ? GL_COLOR_BUFFER_BIT : 0) |
                   ((clearFlags & std::to_underlying(ClearFlags::Depth)) ? GL_DEPTH_BUFFER_BIT : 0) |
                   ((clearFlags & std::to_underlying(ClearFlags::Stencil)) ? GL_STENCIL_BUFFER_BIT : 0);

                if (clearMask != 0) {
                    glClear(clearMask);
                }
            }
        }


    public:



        /**
         * @brief Constructs the backend bound to the engine world.
         *
         * @param engineWorld Engine world providing render resources and targets.
         */
        explicit OpenGLBackend(EngineWorld& engineWorld) : engineWorld_(engineWorld) {}


        /**
         * @brief Begins processing for one render-target batch.
         *
         * @details Binds the framebuffer, validates it in debug builds, and initializes
         * pass-independent GL state such as blending and clear color.
         *
         * @param renderTargetHandle Render-target handle for this batch.
         */
        void beginRenderTargetBatch(const RenderTargetHandle renderTargetHandle) noexcept {

            auto renderTargetEntity = engineWorld_.find<RenderTargetHandle>(renderTargetHandle);

            #ifdef HELIOS_DEBUG
            if (!renderTargetEntity) {
                logger_.error("Missing RenderTargetEntity for handle {0}.", renderTargetHandle.entityId);
                assert(renderTargetEntity && "Missing RenderTargetEntity for handle.");
            }
            #endif

            currentRenderTargetHandle_ = renderTargetHandle;

            const auto renderTargetId = renderTargetEntity->get<OpenGLRenderTargetIdComponent<RenderTargetHandle>>()->value();

            glBindFramebuffer(GL_FRAMEBUFFER, renderTargetId);

            #ifdef HELIOS_DEBUG
            const auto isValidRenderTarget = renderTargetId == 0 ||
                (glIsFramebuffer(renderTargetId) == GL_TRUE && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            if (!isValidRenderTarget) {
                logger_.error("RenderTargetEntity with EntityId {0} undefined.", renderTargetId);
                assert(isValidRenderTarget && "RenderTargetEntity EntityId does not seem to be a valid id.");
            }
            #endif

            auto renderTargetSize = renderTargetEntity->get<Size2DComponent<RenderTargetHandle>>()->value();

            glViewport(0, 0,
                static_cast<int>(renderTargetSize[0]),
                static_cast<int>(renderTargetSize[1])
            );

            clearColor<RenderTargetHandle>(renderTargetEntity);

            // this is equally important for the GlpyhTextRenderer
            // enable blending since the font's fragment shader uses the alpha channel
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        /**
         * @brief Ends processing for one render-target batch.
         *
         * @details Clears current render-target state and resets cached pass/draw uniform values.
         *
         * @param renderTargetHandle Render-target handle for this batch.
         */
        void endRenderTargetBatch(const RenderTargetHandle renderTargetHandle) noexcept {

            currentRenderTargetHandle_ = RenderTargetHandle{};
            passUniformValueBag_.clearValues();
        }

        /**
         * @brief Begins processing for one viewport batch.
         *
         * @details Resolves camera matrices, configures viewport/scissor rectangles, and performs
         * optional clears according to the active render target clear flags.
         *
         * @param viewportHandle Viewport handle for this batch.
         */
        void beginViewportBatch(const ViewportHandle viewportHandle) noexcept {

            auto viewport = engineWorld_.find<ViewportHandle>(viewportHandle);
            auto renderTargetEntity = engineWorld_.find<RenderTargetHandle>(currentRenderTargetHandle_);

            #ifdef HELIOS_DEBUG
            if (!renderTargetEntity) {
                logger_.error("Missing RenderTargetEntity for handle {0}.", renderTargetEntity->handle().entityId);
                assert(renderTargetEntity && "Missing RenderTargetEntity for handle.");
            }
            if (!viewport) {
                logger_.error("Missing Viewport for handle {0}.", viewportHandle.entityId);
                assert(viewport && "Missing Viewport for handle.");
            }
            #endif

            auto vp = viewProjection(*viewport);
            if (!vp) {
                logger_.warn("Could not determine View/Projection-matrices for RenderPass");
                passUniformValueBag_.set<ProjectionMatrixUniform>(helios::math::mat4f{1.0f});
                passUniformValueBag_.set<ViewMatrixUniform>(helios::math::mat4f{1.0f});
            } else {
                passUniformValueBag_.set<ProjectionMatrixUniform>(vp->projectionMatrix);
                passUniformValueBag_.set<ViewMatrixUniform>(vp->viewMatrix);
            }

            auto viewportBounds  = viewport->get<RectComponent<ViewportHandle>>()->value();
            auto renderTargetSize = renderTargetEntity->get<Size2DComponent<RenderTargetHandle>>()->value();

            const auto x = static_cast<int>(renderTargetSize[0] * viewportBounds[0]);
            const auto y = static_cast<int>(renderTargetSize[1] * viewportBounds[1]);
            const auto width = static_cast<int>(renderTargetSize[0] * viewportBounds[2]);
            const auto height = static_cast<int>(renderTargetSize[1] * viewportBounds[3]);


            glViewport(x, y, width, height);
            glScissor(x, y, width, height);
            glEnable(GL_SCISSOR_TEST);

            clearColor<ViewportHandle>(viewport);
        }

        /**
         * @brief Ends processing for one viewport batch.
         *
         * @param viewportHandle Viewport handle for this batch.
         */
        void endViewportBatch(const ViewportHandle viewportHandle) noexcept {
            glDisable(GL_SCISSOR_TEST);
        }

        /**
         * @brief Begins processing for one shader batch.
         *
         * @details Binds the shader program and uploads pass-scope uniforms.
         *
         * @param shaderHandle Shader handle for this batch.
         */
        void beginShaderBatch(ShaderHandle shaderHandle) noexcept {

            auto shaderEntity = engineWorld_.find(shaderHandle);
            if (!shaderEntity) {
                logger_.error("ShaderEntity expected, but not found");
                assert(false && "ShaderEntity not found");
                return;
            }

            currentShaderHandle_ = shaderHandle;

            auto* openglShader = shaderEntity->template get<OpenGLShaderComponent<ShaderHandle>>();
            if (!openglShader) {
                logger_.error("OpenGLShader expected, but not found");
                assert(false && "OpenGLShader not found");
                return;
            }

            glUseProgram(openglShader->programId);
            writeUniformValues<UniformScope::Pass>(*shaderEntity, passUniformValueBag_);
        }

        /**
         * @brief Ends processing for one shader batch.
         *
         * @param handle Shader handle for this batch.
         */
        void endShaderBatch(ShaderHandle handle) noexcept {
            currentShaderHandle_ = ShaderHandle{};
        }

        /**
         * @brief Begins processing for one material batch.
         *
         * @details Loads material-scope values (for example base color) into draw-scope uniforms.
         *
         * @param materialHandle Material handle for this batch.
         */
        void beginMaterialBatch(MaterialHandle materialHandle) noexcept {
            auto materialEntity = engineWorld_.find(materialHandle);
            if (!materialEntity) {
                logger_.error("MaterialEntity expected, but not found");
                assert(false && "MaterialEntity not found");
                return;
            }

            auto* colorComponent = materialEntity->template get<ColorComponent<MaterialHandle>>();
            if (colorComponent) {
                drawUniformValueBag_.set<MaterialBaseColorUniform>(colorComponent->value());
            }

        }

        /**
         * @brief Ends processing for one material batch.
         *
         * @param handle Material handle for this batch.
         */
        void endMaterialBatch(MaterialHandle handle) noexcept {
            // intentionally left empty
        }

        /**
         * @brief Begins processing for one mesh batch.
         *
         * @details Resolves and binds the mesh VAO used for all draw contexts in the batch.
         *
         * @param meshHandle Mesh handle for this batch.
         */
        void beginMeshBatch(MeshHandle meshHandle) noexcept {

            auto meshEntity = engineWorld_.find(meshHandle);
            if (!meshEntity) {
                logger_.error("MeshEntity expected, but not found");
                assert(false && "MeshEntity not found");
                return;
            }

            auto* openglMesh = meshEntity->template get<OpenGLMeshComponent<MeshHandle>>();
            if (!openglMesh) {
                logger_.error("OpenGLMesh expected, but not found");
                assert(false && "OpenGLMesh not found");
                return;
            } else {
                currentOpenGLMesh_ = openglMesh;
                glBindVertexArray(openglMesh->vao);
            }
        }

        /**
         * @brief Ends processing for one mesh batch.
         *
         * @param handle Mesh handle for this batch.
         */
        void endMeshBatch(MeshHandle handle) noexcept {
            currentOpenGLMesh_ = nullptr;
            glBindVertexArray(0);
        }


        /**
         * @brief Renders an instanced batch and individual draw contexts.
         *
         * @details
         * - If `instanceData` is non-empty, uploads instance payload to the active instance VBO
         *   and issues one `glDrawElementsInstanced` call.
         * - Iterates `sceneMemberRenderContexts`, updates draw-scope uniforms per
         *   context, and issues one indexed draw call per member.
         *
         * @tparam THandle Scene member handle type contained in render contexts.
         * @param sceneMemberRenderContexts Non-instanced draw contexts.
         * @param instanceData Optional per-instance payload for instanced rendering.
         */
        template<typename THandle>
        void renderBatch(
            std::span<const SceneMemberRenderContext<THandle>> sceneMemberRenderContexts,
            std::span<const InstanceData> instanceData
            ) noexcept {

            if (!currentOpenGLMesh_) {
                logger_.error("OpenGLMesh expected, but not available");
                return;
            }
            if (!currentShaderHandle_.isValid()) {
                logger_.error("Expected valid currentShaderHandle_, but found {0}.", currentShaderHandle_.entityId);
                return;
            }

            const auto shaderEntity = engineWorld_.find(currentShaderHandle_);

            assert(shaderEntity && "ShaderEntity expected, but not found");
            assert(currentOpenGLMesh_ && "Current OpenGL mesh expected, but not found");

            const auto instanceSize = instanceData.size();

            assert(instanceSize <= 1000000 && "Instance data size seems unreasonably large.");

            if (instanceSize > 0) {

                /**
                 * @todo move to own material scope (colors)
                 */
                writeUniformValues<UniformScope::Draw>(*shaderEntity, drawUniformValueBag_);

                assert(currentOpenGLMesh_->instanceVbo && "Using instancing without configured instanceVbo");
                glBindBuffer(GL_ARRAY_BUFFER, currentOpenGLMesh_->instanceVbo);

                glBufferData(
                    GL_ARRAY_BUFFER,
                    instanceSize * sizeof(InstanceData),
                    instanceData.data(),
                    GL_DYNAMIC_DRAW);


                glDrawElementsInstanced(
                    currentOpenGLMesh_->primitiveType,
                    currentOpenGLMesh_->indexCount,
                    GL_UNSIGNED_INT,
                    nullptr,
                    instanceSize
                );

                glBindBuffer(GL_ARRAY_BUFFER, 0);

            }

            for (auto& renderContext : sceneMemberRenderContexts) {

                drawUniformValueBag_.set<ModelMatrixUniform>(renderContext.worldMatrix);
                writeUniformValues<UniformScope::Draw>(*shaderEntity, drawUniformValueBag_);
                glDrawElements(
                    currentOpenGLMesh_->primitiveType,
                    currentOpenGLMesh_->indexCount,
                    GL_UNSIGNED_INT,
                    nullptr
                );
            }

            drawUniformValueBag_.clearValues();
        }

        /**
         * @brief Applies window hints for an OpenGL core-profile context.
         *
         * @details The backend currently requests OpenGL 4.1 core profile for macOS
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
        [[nodiscard]] bool init() noexcept {

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
        [[nodiscard]] bool isInitialized() const noexcept {
            return isInitialized_;
        }

    };
} // namespace helios::opengl
