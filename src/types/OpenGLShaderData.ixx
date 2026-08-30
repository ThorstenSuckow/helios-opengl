/**
 * @file OpenGLShaderData.ixx
 * @brief Data structure for OpenGL shader data.
 */
module;

#include <cstdint>
#include <vector>

export module helios.opengl.types:OpenGLShaderData;

import :OpenGLViewProjectionData;
import :OpenGLClearColorData;
import helios.opengl.types.OpenGLUniformWriteOperation;

export namespace helios::opengl::types {

    struct OpenGLShaderData {

        std::uint32_t programId = 0;

        std::vector<OpenGLUniformWriteOperation> passWriteOperations{};
        std::vector<OpenGLUniformWriteOperation> materialWriteOperations{};
        std::vector<OpenGLUniformWriteOperation> drawWriteOperations{};

    };


}