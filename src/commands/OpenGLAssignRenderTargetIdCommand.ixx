/**
 * @file OpenGLAssignRenderTargetIdCommand.ixx
 * @brief Command that requests assigning an OpenGL framebuffer id to a render target.
 */
module;

export module helios.opengl.commands.OpenGLAssignRenderTargetIdCommand;

export namespace helios::opengl::commands {

    /**
     * @brief Requests that the OpenGL backend assigns a framebuffer id to a render target.
     *
     * @tparam TRenderTargetHandle Render target handle type.
     */
    template<typename TRenderTargetHandle>
    struct OpenGLAssignRenderTargetIdCommand {

        /**
         * @brief Handle of the render target that receives the id.
         */
        TRenderTargetHandle renderTargetHandle;

    };
}

