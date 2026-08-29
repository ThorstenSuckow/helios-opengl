/**
 * @file OpenGLTextureComponent.ixx
 * @brief Component storing native OpenGL texture id for a texture entity.
 */
module;

export module helios.opengl.components.OpenGLTextureComponent;
export namespace helios::opengl::components {

    /**
     * @brief Binds a texture entity to its linked OpenGL texture.
     *
     * @tparam THandle Texture handle type.
     */
    template<typename TOwnerHandle>
    struct OpenGLTextureComponent  {

        /** @brief Linked OpenGL texture id. */
        unsigned int textureId = 0;

    };
}