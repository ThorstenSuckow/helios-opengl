/**
 * @file OpenGLUniformLocationWriter.ixx
 * @brief Writes scoped uniform values to cached OpenGL uniform locations.
 */
module;

#include <glad/gl.h>
#include <cstddef>

export module helios.opengl.OpenGLUniformLocationWriter;

import helios.engine.rendering.shader.types.UniformSemantics;
import helios.engine.rendering.shader.UniformValueMap;

import helios.engine.rendering.shader.types.ShaderHandle;

import helios.opengl.components.OpenGLUniformLocationComponent;

using namespace helios::engine::rendering::shader;
using namespace helios::engine::rendering::shader::types;
export namespace helios::opengl {

    /**
     * @brief Applies values from a uniform value map to OpenGL shader uniforms.
     */
    class OpenGLUniformLocationWriter {

    public:
        /**
         * @brief Writes all available uniform values for a specific scope.
         *
         * @details Iterates cached uniform locations by semantic index and submits
         * matching values from `uniformValueMap` using the corresponding OpenGL
         * uniform call. Entries with location `-1` are skipped.
         *
         * @tparam TUniformScope Uniform lifetime scope tag (for example pass or draw).
         * @param ulc Cached OpenGL uniform locations for the shader and scope.
         * @param uniformValueMap Scope-specific value map used as upload source.
         */
        template<typename TUniformScope>
        void writeUniformValues(
            const components::OpenGLUniformLocationComponent<ShaderHandle, TUniformScope>* ulc,
            const UniformValueMap<TUniformScope>& uniformValueMap
        ) const noexcept {

             for (std::size_t i = 0; i < ulc->locations.size(); ++i) {
                GLint location = ulc->locations[i];
                if (location == -1) {
                    continue;
                }

                auto semantics = static_cast<UniformSemantics>(i);

                switch (semantics) {
                    case UniformSemantics::ViewMatrix:
                    case UniformSemantics::ProjectionMatrix:
                    case UniformSemantics::ModelMatrix:
                        if (const auto* mat4f_ptr = uniformValueMap.mat4f_ptr(semantics)) {
                            glUniformMatrix4fv(location, 1, false, mat4f_ptr);
                        }
                        break;
                    case UniformSemantics::MaterialBaseColor:
                    case UniformSemantics::TextColor:
                        if (const auto* vec4f_ptr = uniformValueMap.vec4f_ptr(semantics)) {
                            glUniform4fv(location, 1, vec4f_ptr);
                        }
                        break;
                    case UniformSemantics::TextTexture:
                        if (const auto* int_ptr = uniformValueMap.int_ptr(UniformSemantics::TextTexture)) {
                            glUniform1i(location, *int_ptr);
                        }
                    break;

                }

            }

        }
    };

}