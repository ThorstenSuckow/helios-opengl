/**
 * @file OpenGLTextureComponent.ixx
 * @brief Component storing native OpenGL texture id for a texture entity.
 */
module;

export module helios.opengl.components.OpenGLTextureComponent;

import helios.engine.rendering.texture.concepts;

using namespace helios::engine::rendering::texture::concepts;
export namespace helios::opengl::components {

    /**
     * @brief Binds a texture entity to its linked OpenGL texture.
     *
     * @tparam THandle Texture handle type.
     */
    template<typename THandle>
    requires IsTextureHandle<THandle>
    struct OpenGLTextureComponent  {

        /** @brief Linked OpenGL texture id. */
        unsigned int textureId = 0;

    };
}