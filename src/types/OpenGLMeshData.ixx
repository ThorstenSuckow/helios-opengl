/**
 * @file OpenGLMeshData.ixx
 * @brief Data structure for OpenGL mesh data.
 */
module;

#include <glad/gl.h>
#include <cstdint>
#include <cstddef>

export module helios.opengl.types:OpenGLMeshData;

import helios.math;

export namespace helios::opengl::types {

    struct OpenGLMeshData {

        /**
         * @brief Vertex Array Object handle.
         */
        std::uint32_t vao;

        /**
         * @brief Vertex Buffer Object handle.
         */
        std::uint32_t vbo;

        /**
         * @brief Element Buffer Object handle.
         */
        std::uint32_t ebo;

        /**
         * @brief Instance Buffer Object handle, if any. Will be 0 if no instancing is cosnidered.
         */
        std::uint32_t instanceVbo;

        /**
         * @brief Number of indices used for indexed draw calls.
         */
        std::uint32_t indexCount;

        /**
         * @brief Primitive topology used for rendering this mesh.
         */
        GLenum primitiveType;
        
    };


}