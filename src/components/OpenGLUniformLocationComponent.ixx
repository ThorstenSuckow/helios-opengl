/**
 * @file OpenGLUniformLocationComponent.ixx
 * @brief OpenGL component storing cached uniform locations by semantic index.
 */
module;

#include <glad/gl.h>
#include <array>
#include <cstddef>
#include <utility>

export module helios.opengl.components.OpenGLUniformLocationComponent;

import helios.engine.rendering.shader.types.UniformSemantics;

using namespace helios::engine::rendering::shader::types;
export namespace helios::opengl::components {

    /**
     * @brief Stores resolved OpenGL uniform locations for one shader/resource handle.
     * @tparam THandle Owner handle type used by ECS composition.
     */
    template<typename THandle>
    struct OpenGLUniformLocationComponent {

        /**
         * @brief Number of supported uniform semantics.
         */
        static inline constexpr auto UniformSemanticsCount = static_cast<std::size_t>(
            std::to_underlying(UniformSemantics::size_)
        );

        /**
         * @brief Cached uniform locations indexed by `UniformSemantics`.
         * @details Entries default to `-1` to represent unresolved/missing uniforms.
         */
        std::array<GLint, UniformSemanticsCount> locations{-1};

    };


}