/**
 * @file OpenGLUniformWriter.ixx
 * @brief Applies prepared uniform write operations to the currently bound OpenGL program.
 */
module;

#include <glad/gl.h>
#include <span>
#include <cassert>

export module helios.opengl.OpenGLUniformWriter;

import helios.opengl.types.OpenGLUniformWriteOperation;
import helios.engine.rendering.shader;
import helios.math.types;

import helios.opengl.types;

using namespace helios::opengl::types;
using namespace helios::engine::rendering::shader;
using namespace helios::engine::rendering::shader::types;
export namespace helios::opengl {

    /**
     * @brief Executes typed uniform uploads based on pre-resolved OpenGL locations.
     */
    class OpenGLUniformWriter {


    public:

        /**
         * @brief Writes uniform values for a specific scope to OpenGL.
         *
         * @details Iterates over prepared write operations, skips invalid locations
         * (`-1`), maps semantics to supported uniform types, and submits the
         * matching `glUniform*` call when the value exists in `uniformValueBag`.
         *
         * @tparam TUniformScope Uniform lifetime scope tag (for example pass or draw).
         * @param operations Sequence of resolved OpenGL uniform write operations.
         * @param uniformValueBag Typed uniform values used as upload source.
         */
        template<typename TUniformScope>
        static void write(
            std::span<OpenGLUniformWriteOperation> operations,
            UniformValueBag<TUniformScope>& uniformValueBag
        ) noexcept {

            for (std::size_t i = 0; i < operations.size(); ++i) {
                auto& op = operations[i];
                GLint location = op.location;
                if (location == -1) {
                    continue;
                }

                auto semantics = static_cast<UniformSemantics>(op.semantics);
                switch (semantics) {
                    case UniformSemantics::ViewMatrix:
                        if (uniformValueBag.template has<ViewMatrixUniform>()) {
                            glUniformMatrix4fv(location, 1, false,
                                helios::math::value_ptr(uniformValueBag.template get<ViewMatrixUniform>()));
                        }
                    break;
                    case UniformSemantics::ProjectionMatrix:
                        if (uniformValueBag.template has<ProjectionMatrixUniform>()) {
                            glUniformMatrix4fv(location, 1, false,
                                helios::math::value_ptr(uniformValueBag.template get<ProjectionMatrixUniform>()));
                        }
                    break;
                    case UniformSemantics::ModelMatrix:
                        if (uniformValueBag.template has<ModelMatrixUniform>()) {
                            glUniformMatrix4fv(location, 1, false,
                                helios::math::value_ptr(uniformValueBag.template get<ModelMatrixUniform>()));
                        }
                        break;
                    case UniformSemantics::MaterialBaseColor:
                        if (uniformValueBag.template has<MaterialBaseColorUniform>()) {
                            glUniform4fv(location, 1,
                                helios::math::value_ptr(uniformValueBag.template get<MaterialBaseColorUniform>()));
                        }
                    break;
                    default:
                        assert(false && "Missing support for Uniform");
                        break;
                }

            }

        }


    };
}
