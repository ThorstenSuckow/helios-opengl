/**
 * @file OpenGLRenderTargetManager.ixx
 * @brief OpenGL manager that assigns framebuffer ids to render targets.
 */
module;

#include <cassert>
#include <utility>
#include <vector>

export module helios.opengl.OpenGLRenderTargetManager;

import helios.core.log;
import helios.ecs;

import helios.opengl.commands.OpenGLAssignRenderTargetIdCommand;
import helios.opengl.components.OpenGLRenderTargetIdComponent;

using namespace helios::core::log;
using namespace helios::opengl::commands;
using namespace helios::opengl::components;
using namespace helios::ecs;

#define HELIOS_LOG_SCOPE "helios::opengl::OpenGLRenderTargetManager"
export namespace helios::opengl {


    template<typename TRenderTargetHandle>
    class OpenGLRenderTargetManager {

        using Command = OpenGLAssignRenderTargetIdCommand<TRenderTargetHandle>;

        /**
         * @brief Pending commands, processed in `execute(...)`.
         */
        std::vector<Command> commands_;

        inline static const Logger& logger_ = LogManager::loggerForScope(HELIOS_LOG_SCOPE);

    public:

        /**
         * @brief Assigns the default framebuffer id (0) to all queued render targets.
         */
        bool execute(entity::EntityManager<TRenderTargetHandle>& entityManager) noexcept {

            for (const auto& command : commands_) {
                auto renderTargetEntity = entityManager.entity(command.renderTargetHandle);

                if (!renderTargetEntity) {
                    logger_.error("Could not find render target entity");
                    assert(false && "Could not find render target entity");
                    continue;
                }

                // idempotent: the same window may be requested in consecutive frames
                renderTargetEntity->template getOrAdd<OpenGLRenderTargetIdComponent>(0);
            }

            commands_.clear();
            return true;
        }

        /**
         * @brief Queues a command for processing in `execute(...)`.
         */
        bool submit(Command&& command) noexcept {
            commands_.push_back(std::move(command));
            return true;
        }

        /**
         * @brief Registers the command handler in the command registry.
         */
        bool init(ecs::command::CommandHandlerRegistry& commandHandlerRegistry) noexcept {
            commandHandlerRegistry.template registerHandler<Command>(*this);
            return true;
        }

        void reset() {
            /*intentionally left noop*/
        }

    };

}

