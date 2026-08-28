/**
 * @file OpenGLViewProjectionData.ixx
 * @brief Data structure for OpenGL view and projection matrices.
 */
module;

#include <cstdint>

export module helios.opengl.types:OpenGLViewProjectionData;

import helios.math;

export namespace helios::opengl::types {

    struct OpenGLViewProjectionData {

        math::mat4f viewMatrix;
        math::mat4f projectionMatrix;

    };


}