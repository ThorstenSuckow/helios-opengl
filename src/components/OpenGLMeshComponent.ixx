/**
 * @file OpenGLMeshComponent.ixx
 * @brief OpenGL mesh component storing GPU object handles and draw metadata.
 */
module;


export module helios.opengl.components.OpenGLMeshComponent;


import helios.opengl.types;


export namespace helios::opengl::components {

    template<typename TOwnerHandle>
    struct OpenGLMeshComponent {

        types::OpenGLMeshData data;


    };

}