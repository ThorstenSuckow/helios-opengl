/**
 * @file OpenGLViewportData.ixx
 * @brief Data structure for OpenGL viewport data.
 */
module;

#include <cstdint>
#include <optional>
#include <array>

export module helios.opengl.types:OpenGLViewportData;

import :OpenGLViewProjectionData;
import :OpenGLClearColorData;

export namespace helios::opengl::types {

    struct OpenGLViewportData {

        std::optional<OpenGLViewProjectionData> viewProjectionData;
        std::array<float, 4> viewportBounds = {0.0f, 0.0f, 0.0f, 0.0f};

        OpenGLClearColorData clearColorData{};

    };


}