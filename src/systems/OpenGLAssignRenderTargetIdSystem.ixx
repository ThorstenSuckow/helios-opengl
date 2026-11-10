/**
 * @file OpenGLAssignRenderTargetIdSystem.ixx
 * @brief System that requests OpenGL framebuffer ids for the render targets of windows being created.
 */
module;

export module helios.opengl.systems.OpenGLAssignRenderTargetIdSystem;

import helios.ecs.command;
import helios.ecs.entity.EntityWorld;
import helios.ecs.entity.QueryAccessSet;
import helios.ecs.entity.query.Query;

import helios.engine.platform.window.components.WindowCreateRequestComponent;
import helios.engine.rendering.common.components;
import helios.opengl.commands.OpenGLAssignRenderTargetIdCommand;

using namespace helios::engine::platform::window::components;
using namespace helios::opengl::commands;

export namespace helios::opengl::systems {


    template<typename TWindowHandle, typename TRenderHandles>
    class OpenGLAssignRenderTargetIdSystem {

        template<typename TRead, typename TWrite, typename TFilter>
        using Query = ecs::entity::query::Query<TWindowHandle, TRead, TWrite, TFilter>;

        template<typename ... TReads>
        using Read = ecs::entity::ReadSet<TReads...>;

        template<typename ... TWrites>
        using Write = ecs::entity::WriteSet<TWrites...>;

        using RenderTargetHandle = typename TRenderHandles::RenderTargetHandle;
        using RenderTargetBindingComponent =
            engine::rendering::common::components::RenderTargetBindingComponent<TRenderHandles>;

    public:

        using CommandBuffer = ecs::command::TypedCommandBuffer<OpenGLAssignRenderTargetIdCommand<RenderTargetHandle>>;

        void update(
            Query<
                Read<WindowCreateRequestComponent, RenderTargetBindingComponent>,
                Write<>,
                ecs::entity::query::Filter<ecs::entity::query::IsActive>
            > query,
            CommandBuffer& cmdBuffer
        ) noexcept {

            for (auto [entity, request, binding] : query) {
                cmdBuffer.template add<OpenGLAssignRenderTargetIdCommand<RenderTargetHandle>>(
                    binding->targetHandle()
                );
            }
        }

    };

}

