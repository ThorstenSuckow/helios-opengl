/**
 * @file OpenGLBackend.ixx
 * @brief Draft OpenGL backend for render pass execution.
 */
module;

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <cassert>
#include <utility>
#include "helios-opengl-config.h"


export module helios.opengl.OpenGLBackend;

import helios.math.types;

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
using namespace helios::math;
using namespace helios::engine::rendering;
using namespace helios::engine::rendering::shader::types;
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

#define HELIOS_LOG_SCOPE "helios::opengl"
export namespace helios::opengl {


    /**
     * @brief Applies render-pass state and draw execution via OpenGL.
     *
     * @details
     * `OpenGLBackend` is intentionally thin and stateful: it references existing
     * worlds and translates ECS render data into OpenGL state changes.
     */
    class OpenGLBackend {

    private:

        RenderResourceWorld& renderResourcesWorld_;

        RenderTargetWorld& renderTargetsWorld_;

        bool isInitialized_ = false;

        inline static const helios::engine::util::log::Logger& logger_ = helios::engine::util::log::LogManager::loggerForScope(
            HELIOS_LOG_SCOPE
        );

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
         * @tparam THandle Scene member handle type.
         * @param renderContext Per-member render context.
         *
         * @note Placeholder hook for concrete draw submission.
         */
        template<typename THandle>
        void doRender(const SceneMemberRenderContext<THandle>& renderContext)  {

        }

        /**
         * @brief Ends the current render pass.
         *
         * @details
         * Restores transient state that was enabled in `beginRenderPass`.
         *
         * @param renderPassContext Render pass target context.
         */
        void endRenderPass(const RenderPassContext& renderPassContext) {

            glDisable(GL_SCISSOR_TEST);

        }

        /**
         * @brief Applies window hints for an OpenGL core profile context.
         *
         * @details
         * The backend currently requests OpenGL 4.1 core profile for MacOS compatibility.
         */
        void provideWindowHints() noexcept {

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        }

        /**
         * @brief Initializes GL function pointers through GLFW.
         *
         * @return `true` if loading succeeded, otherwise `false`.
         *
         * @pre A valid, current OpenGL context exists on the calling thread.
         * @post `isInitialized()` returns `true` on success.
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
         * @brief Reports whether GL function loading was completed.
         */
        bool isInitialized() {
            return isInitialized_;
        }




    };
} // namespace helios::opengl
