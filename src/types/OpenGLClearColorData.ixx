/**
 * @file OpenGLClearColorData.ixx
 * @brief Data structure for OpenGL clear color and clear flags.
 */
module;


#include <array>

export module helios.opengl.types:OpenGLClearColorData;


export namespace helios::opengl::types {

    struct OpenGLClearColorData {
        std::array<float, 4> color = {0.0f, 0.0f, 0.0f, 1.0f};
        unsigned int clearFlags = 0;
    };

}