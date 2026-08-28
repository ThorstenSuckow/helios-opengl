/**
 * @file OpenGLTextureData.ixx
 * @brief Data structure for OpenGL texture data.
 */
module;

#include <cstdint>

export module helios.opengl.types:OpenGLTextureData;


export namespace helios::opengl::types {

    struct OpenGLTextureData {

        std::uint32_t textureId = 0;
    };


}