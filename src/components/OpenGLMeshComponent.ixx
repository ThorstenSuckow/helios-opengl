/**
 * @file OpenGLMeshComponent.ixx
 * @brief OpenGL mesh component storing buffer object handles.
 */
module;

export module helios.opengl.components.OpenGLMeshComponent;

export namespace helios::opengl::components {

    /**
     * @brief Stores OpenGL object IDs required to render a mesh.
     * @tparam TOwnerHandle Owner handle type used by ECS composition.
     */
    template<typename TOwnerHandle>
    struct OpenGLMeshComponent {

        /**
         * @brief Vertex Array Object handle.
         */
        unsigned int vao;

        /**
         * @brief Vertex Buffer Object handle.
         */
        unsigned int vbo;

        /**
         * @brief Element Buffer Object handle.
         */
        unsigned int ebo;

    };

}