/**
 * @file OpenGLRenderTargetData.ixx
 * @brief Data structure for OpenGL render target data.
 */
module;

#include <cstdint>
#include <array>

export module helios.opengl.types:OpenGLRenderTargetData;

import :OpenGLClearColorData;

export namespace helios::opengl::types {

    struct OpenGLRenderTargetData {
        std::uint32_t renderTargetId = 0;
        std::array<float, 2> renderTargetSize = {0.0f, 0.0f};
        OpenGLClearColorData clearColorData{};
    };


}