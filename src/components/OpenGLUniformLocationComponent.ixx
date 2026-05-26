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
     * @tparam TUniformScope Uniform lifetime scope tag (for example pass or draw scope).
     */
    template<typename THandle, typename TUniformScope>
    struct OpenGLUniformLocationComponent {

        /**
         * @brief Number of supported uniform semantics.
         */
        static inline constexpr auto UniformSemanticsCount = static_cast<std::size_t>(
            std::to_underlying(UniformSemantics::size_)
        );

        /**
         * @brief Cached uniform locations indexed by `UniformSemantics`.
         */
        std::array<GLint, UniformSemanticsCount> locations{-1};

        /**
         * @brief Constructs the cache and initializes all entries to "not found" (`-1`).
         */
        OpenGLUniformLocationComponent() {
            locations.fill(-1);
        }
    };



}