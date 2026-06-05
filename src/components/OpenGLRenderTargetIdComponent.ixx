/**
 * @file OpenGLRenderTargetIdComponent.ixx
 * @brief OpenGL renderTarget ID component alias.
 */
module;

#include <cstdint>

export module helios.opengl.components.OpenGLRenderTargetIdComponent;

import helios.engine.core.components.NumericValueComponent;

using namespace helios::engine::core::components;
export namespace helios::opengl::components {

    /** @brief Domain tag for OpenGL renderTarget object identifiers. */
    struct OpenGLRenderTargetIdDomain {};

    /**
     * @brief Stores an OpenGL renderTarget object ID.
     *
     * @tparam THandle Owning entity handle type.
     */
    template<typename THandle>
    using OpenGLRenderTargetIdComponent = NumericValueComponent<OpenGLRenderTargetIdDomain, uint32_t, THandle>;

}