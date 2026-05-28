/**
 * @file OpenGLUniformWriteOperationsComponent.ixx
 * @brief OpenGL component storing prepared uniform write operations.
 */
module;


#include <vector>


export module helios.opengl.components.OpenGLUniformWriteOperationsComponent;

import helios.opengl.types.OpenGLUniformWriteOperation;
import helios.engine.rendering.shader.types.UniformSemantics;

using namespace helios::opengl::types;
export namespace helios::opengl::components {

    /**
     * @brief Stores prepared OpenGL uniform write operations for one shader/scope pair.
     * @tparam THandle Owner handle type used by ECS composition.
     * @tparam TUniformScope Uniform lifetime scope tag (for example pass or draw scope).
     */
    template<typename THandle, typename TUniformScope>
    struct OpenGLUniformWriteOperationsComponent {

        /**
         * @brief Ordered uniform write operations with resolved locations.
         */
        std::vector<OpenGLUniformWriteOperation> operations;

    };



}