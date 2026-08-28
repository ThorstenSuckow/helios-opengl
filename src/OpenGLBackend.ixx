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

import helios.core.log;

import helios.engine.rendering.viewport.ViewportEntity;

import helios.engine.rendering.renderTarget.RenderTargetEntity;
import helios.engine.rendering.common.types;

import helios.opengl.OpenGLUniformWriter;
import helios.opengl.types;

import helios.engine.rendering.mesh;
import helios.engine.rendering.shader;
import helios.engine.rendering.material;
import helios.engine.rendering.texture;
import helios.engine.rendering.renderTarget;
import helios.engine.rendering.viewport;
import helios.engine.util.Colors;

import helios.engine.scene.components;
import helios.engine.scene.types;

import helios.opengl.types;

using namespace helios::engine::rendering;
using namespace helios::engine::rendering::shader;
using namespace helios::engine::rendering::shader::types;
using namespace helios::opengl;
using namespace helios::opengl::types;
using namespace helios::engine::rendering::material::types;
using namespace helios::engine::rendering::texture::types;
using namespace helios::engine::rendering::common::types;
using namespace helios::engine::rendering::mesh::types;
using namespace helios::engine::rendering::renderTarget;
using namespace helios::engine::rendering::renderTarget::types;
using namespace helios::engine::rendering::viewport;
using namespace helios::engine::rendering::viewport::types;
using namespace helios::engine::scene::types;
using namespace helios::core::log;

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
        inline static const helios::core::log::Logger& logger_ = helios::core::log::LogManager::loggerForScope(
            HELIOS_LOG_SCOPE
        );


        types::OpenGLRenderTargetData currentRenderTargetData_ = {};
        types::OpenGLShaderData currentShaderData_ = {};
        types::OpenGLMeshData currentMeshData_ = {};

        /**
         * @brief Cached pass-scope uniforms (typically view/projection).
         */
        UniformValueBag<UniformScope::Pass> passUniformValueBag_{};

        /**
         * @brief Cached draw-scope uniforms (for example model matrix).
         */
        UniformValueBag<UniformScope::Draw> drawUniformValueBag_{};

        /**
         * @brief Cached material-scope uniforms (for example material color).
         */
        UniformValueBag<UniformScope::Material> materialUniformValueBag_{};

        /**
         * @brief Applies clear color and clear mask based on optional components.
         */
        void clearColor(const types::OpenGLClearColorData& clearColorData) noexcept {

            const auto clearColor = clearColorData.color;
            glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

            const auto clearFlags = clearColorData.clearFlags;
            const auto clearMask = ((clearFlags & std::to_underlying(ClearFlags::Color)) ? GL_COLOR_BUFFER_BIT : 0) |
               ((clearFlags & std::to_underlying(ClearFlags::Depth)) ? GL_DEPTH_BUFFER_BIT : 0) |
               ((clearFlags & std::to_underlying(ClearFlags::Stencil)) ? GL_STENCIL_BUFFER_BIT : 0);

            if (clearMask != 0) {
                glClear(clearMask);
            }
        }


    public:


        /**
         * @brief Begins processing for one render-target batch.
         *
         * @param renderTargetData
         */
        void beginRenderTargetBatch(const types::OpenGLRenderTargetData& renderTargetData) noexcept {

            currentRenderTargetData_ = renderTargetData;

            const auto renderTargetId = renderTargetData.renderTargetId;

            glBindFramebuffer(GL_FRAMEBUFFER, renderTargetId);

            #ifdef HELIOS_DEBUG
            const auto isValidRenderTarget = renderTargetId == 0 ||
                (glIsFramebuffer(renderTargetId) == GL_TRUE && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            if (!isValidRenderTarget) {
                logger_.error("RenderTargetEntity with EntityId {0} undefined.", renderTargetId);
                assert(isValidRenderTarget && "RenderTargetEntity EntityId does not seem to be a valid id.");
            }
            #endif

            auto renderTargetSize = renderTargetData.renderTargetSize;

            glViewport(0, 0,
                static_cast<int>(renderTargetSize[0]),
                static_cast<int>(renderTargetSize[1])
            );

            clearColor(renderTargetData.clearColorData);

            // this is equally important for the GlpyhTextRenderer
            // enable blending since the font's fragment shader uses the alpha channel
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        /**
         * @brief Ends processing for one render-target batch.
         */
        void endRenderTargetBatch() noexcept {
            currentRenderTargetData_ = types::OpenGLRenderTargetData{};
            passUniformValueBag_.clearValues();
        }

        /**
         * @brief Begins processing for one viewport batch.
         *
         * @param viewportData
         */
        void beginViewportBatch(const types::OpenGLViewportData& viewportData) noexcept {


            auto vp = viewportData.viewProjectionData;
            if (!vp) {
                logger_.warn("Could not determine View/Projection-matrices for RenderPass");
                passUniformValueBag_.set<ProjectionMatrixUniform>(helios::math::mat4f{1.0f});
                passUniformValueBag_.set<ViewMatrixUniform>(helios::math::mat4f{1.0f});
            } else {
                passUniformValueBag_.set<ProjectionMatrixUniform>(vp->projectionMatrix);
                passUniformValueBag_.set<ViewMatrixUniform>(vp->viewMatrix);
            }

            auto viewportBounds  = viewportData.viewportBounds;
            auto renderTargetSize = currentRenderTargetData_.renderTargetSize;

            const auto x = static_cast<int>(renderTargetSize[0] * viewportBounds[0]);
            const auto y = static_cast<int>(renderTargetSize[1] * viewportBounds[1]);
            const auto width = static_cast<int>(renderTargetSize[0] * viewportBounds[2]);
            const auto height = static_cast<int>(renderTargetSize[1] * viewportBounds[3]);


            glViewport(x, y, width, height);
            glScissor(x, y, width, height);
            glEnable(GL_SCISSOR_TEST);

            clearColor(viewportData.clearColorData);
        }

        /**
         * @brief Ends processing for one viewport batch.
         *
         * @param viewportHandle Viewport handle for this batch.
         */
        void endViewportBatch() noexcept {
            glDisable(GL_SCISSOR_TEST);
        }

        /**
         * @brief Begins processing for one shader batch.
         *
         * @param shaderData
         */
        void beginShaderBatch(const types::OpenGLShaderData& shaderData) noexcept {

            currentShaderData_ = shaderData;

            glUseProgram(shaderData.programId);

            OpenGLUniformWriter::write(shaderData.uniformWriteOperations, passUniformValueBag_);
        }

        /**
         * @brief Ends processing for one shader batch.
         */
        void endShaderBatch() noexcept {
            currentShaderData_ = types::OpenGLShaderData{};
        }

        /**
         * @brief Begins processing for one texture batch.
         *
         * @param textureData
         */
        void beginTextureBatch(const types::OpenGLTextureData& textureData) noexcept {
            glBindTexture(GL_TEXTURE_2D, textureData.textureId);
        }

        /**
         * @brief Ends the texture batch und unbinds the current texture.
         */
        void endTextureBatch() noexcept {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        /**
         * @brief Begins processing for one material batch.
         *
         * @param materialData
         */
        void beginMaterialBatch(const types::OpenGLMaterialData& materialData) noexcept {
            if (materialData.baseColor) {
                materialUniformValueBag_.set<MaterialBaseColorUniform>(*materialData.baseColor);
                OpenGLUniformWriter::write(currentShaderData_.uniformWriteOperations, materialUniformValueBag_);
            }
        }

        /**
         * @brief Ends processing for one material batch.
         */
        void endMaterialBatch() noexcept {
            materialUniformValueBag_.clearValues();
        }

        /**
         * @brief Begins processing for one mesh batch.
         *
         * @details Resolves and binds the mesh VAO used for all draw contexts in the batch.
         *
         * @param meshData
         */
        void beginMeshBatch(const types::OpenGLMeshData& meshData) noexcept {
            currentMeshData_ = meshData;
            glBindVertexArray(meshData.vao);
        }

        /**
         * @brief Ends processing for one mesh batch.
         */
        void endMeshBatch() noexcept {
            currentMeshData_ = types::OpenGLMeshData{};
            glBindVertexArray(0);
        }


        /**
         * @brief Renders non-instanced draw contexts.
         *
         * @details Iterates `sceneMemberRenderContexts`, updates draw-scope
         * uniforms per context, and issues one indexed draw call per member.
         *
         * @tparam THandle Scene member handle type contained in render contexts.
         * @param sceneMemberRenderContexts Non-instanced draw contexts.
         */
        void renderBatch(const std::span<const DrawContext> sceneMemberRenderContexts) noexcept {


            for (auto& renderContext : sceneMemberRenderContexts) {

                drawUniformValueBag_.set<ModelMatrixUniform>(renderContext.worldMatrix);
                OpenGLUniformWriter::write(currentShaderData_.uniformWriteOperations, drawUniformValueBag_);
                glDrawElements(
                    currentMeshData_.primitiveType,
                    currentMeshData_.indexCount,
                    GL_UNSIGNED_INT,
                    nullptr
                );
            }

            drawUniformValueBag_.clearValues();
        }

        /**
         * @brief Renders one instanced draw call for the provided instance payload.
         *
         * @details Uploads `instanceData` to the active instance VBO and submits
         * one `glDrawElementsInstanced` call. Returns early when the input span is empty.
         *
         * @tparam THandle Scene member handle type used by `InstanceData`.
         * @param instanceData Per-instance payload for instanced rendering.
         */
        void renderBatch(const std::span<const InstanceData> instanceData) const noexcept {

            const auto instanceSize = instanceData.size();

            assert(instanceSize <= 1'000'000 && "Instance data size seems unreasonably large.");

            if (instanceSize <= 0) {
                return;
            }

            assert(currentMeshData_.instanceVbo && "Using instancing without configured instanceVbo");
            glBindBuffer(GL_ARRAY_BUFFER, currentMeshData_.instanceVbo);

            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(instanceSize * sizeof(InstanceData)),
                instanceData.data(),
                GL_DYNAMIC_DRAW);


            glDrawElementsInstanced(
                currentMeshData_.primitiveType,
                currentMeshData_.indexCount,
                GL_UNSIGNED_INT,
                nullptr,
                instanceSize
            );

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        /**
         * @brief Applies window hints for an OpenGL core-profile context.
         *
         * @details The backend currently requests OpenGL 4.1 core profile for macOS
         * compatibility.
         */
        bool configureWindowCreationHints() noexcept {

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            return true;
        }

        /**
         * @brief Initializes OpenGL function pointers through GLFW.
         *
         * @pre A valid, current OpenGL context exists on the calling thread.
         * @post `isInitialized()` returns `true` on success.
         * @return `true` if loading succeeded, otherwise `false`.
         */
        [[nodiscard]] bool finalizeSetup() noexcept {

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
