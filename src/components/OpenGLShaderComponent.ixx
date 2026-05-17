/**
 * @file OpenGLShaderComponent.ixx
 * @brief Component storing native OpenGL program id for a shader entity.
 */
module;

export module helios.opengl.components.OpenGLShaderComponent;

import helios.engine.rendering.shader.concepts.IsShaderHandle;

using namespace helios::engine::rendering::shader::concepts;
export namespace helios::opengl::components {

    /**
     * @brief Binds a shader entity to its linked OpenGL program.
     *
     * @tparam THandle Shader handle type.
     */
    template<typename THandle>
    requires IsShaderHandle<THandle>
    struct OpenGLShaderComponent  {

        /** @brief Linked OpenGL program id. */
        unsigned int programId = 0;

    };
}