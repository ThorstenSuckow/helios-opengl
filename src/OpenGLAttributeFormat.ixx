module;

#include <cstdint>
#include <utility>

export module helios.opengl.OpenGLAttributeFormat;

import helios.engine.rendering.mesh.types.VertexAttributeType;

export namespace helios::opengl {


    /**
     * @brief Derived OpenGL attribute layout information for one engine attribute type.
     */
    struct OpenGLAttributeFormat {
        std::uint32_t locationCount;
        std::uint32_t componentsPerLocation;
        std::uint32_t locationStrideBytes;
    };


    /**
     * @brief Maps engine attribute type to OpenGL attribute format details.
     *
     * @param attributeType Engine-side vertex attribute type.
     * @return Expanded format used to configure one or multiple attribute locations.
     */
    [[nodiscard]] OpenGLAttributeFormat fromAttributeType(
        helios::engine::rendering::mesh::types::VertexAttributeType attributeType) noexcept {

        switch (attributeType) {
            case helios::engine::rendering::mesh::types::VertexAttributeType::Float:
                return OpenGLAttributeFormat{1, 1, 0};
            case helios::engine::rendering::mesh::types::VertexAttributeType::Vec2f:
                return OpenGLAttributeFormat{1, 2, 0};
            case helios::engine::rendering::mesh::types::VertexAttributeType::Vec3f:
                return OpenGLAttributeFormat{1, 3, 0};
            case helios::engine::rendering::mesh::types::VertexAttributeType::Vec4f:
                return OpenGLAttributeFormat{1, 4, 0};
            case helios::engine::rendering::mesh::types::VertexAttributeType::Mat4f:
                return OpenGLAttributeFormat{4, 4, sizeof(float) * 4};
            default:
                std::unreachable();
        }
;
    }
}
