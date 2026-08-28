/**
 * @file OpenGLMaterialData.ixx
 * @brief Data structure for OpenGL material data.
 */
module;

#include <optional>

export module helios.opengl.types:OpenGLMaterialData;

import helios.math;

export namespace helios::opengl::types {

    struct OpenGLMaterialData {

        std::optional<math::vec4f> baseColor;

    };


}