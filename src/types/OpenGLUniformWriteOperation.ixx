/**
 * @file OpenGLUniformWriteOperation.ixx
 * @brief Operation descriptor for writing one semantic uniform to OpenGL.
 */
module;

#include <glad/gl.h>

export module helios.opengl.types.OpenGLUniformWriteOperation;

import helios.engine.rendering.shader.types.UniformSemantics;

using namespace helios::engine::rendering::shader::types;
export namespace helios::opengl::types {

    /**
     * @brief Describes a single uniform write target in an OpenGL program.
     */
    struct OpenGLUniformWriteOperation {
        /**
         * @brief Semantic identifier of the uniform value to upload.
         */
        UniformSemantics semantics;

        /**
         * @brief OpenGL uniform location (`-1` means unresolved/not found).
         */
        GLint location;
    };

}