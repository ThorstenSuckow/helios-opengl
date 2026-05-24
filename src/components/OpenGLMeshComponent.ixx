/**
 * @file OpenGLMeshComponent.ixx
 * @brief OpenGL mesh component storing GPU object handles and draw metadata.
 */
module;

#include <cstddef>

export module helios.opengl.components.OpenGLMeshComponent;

import helios.engine.rendering.mesh.types;

using namespace helios::engine::rendering::mesh::types;
export namespace helios::opengl::components {

    /**
     * @brief Stores OpenGL object IDs and draw information for a mesh.
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

        /**
         * @brief Number of indices used for indexed draw calls.
         */
        std::size_t indexCount;

        /**
         * @brief Primitive topology used for rendering this mesh.
         */
        PrimitiveType primitiveType;

    };

}