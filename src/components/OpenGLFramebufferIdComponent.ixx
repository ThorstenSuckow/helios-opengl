/**
 * @file OpenGLFramebufferIdComponent.ixx
 * @brief OpenGL framebuffer ID component alias.
 */
module;

#include <cstdint>

export module helios.opengl.components.OpenGLFramebufferIdComponent;

import helios.engine.core.components.NumericValueComponent;

using namespace helios::engine::core::components;
export namespace helios::opengl::components {

    /** @brief Domain tag for OpenGL framebuffer object identifiers. */
    struct OpenGLFramebufferIdDomain {};

    /**
     * @brief Stores an OpenGL framebuffer object ID.
     *
     * @tparam THandle Owning entity handle type.
     */
    template<typename THandle>
    using OpenGLFramebufferIdComponent = NumericValueComponent<OpenGLFramebufferIdDomain, THandle, uint32_t>;

}