/**
 * @file OpenGLEnumMapper.ixx
 * @brief Maps engine rendering enums to OpenGL GLenum constants.
 */
module;

#include <algorithm>
#include <glad/gl.h>

export module helios.opengl.OpenGLEnumMapper;

import helios.engine.rendering.mesh.types.PrimitiveType;
import helios.engine.rendering.mesh.types.VertexAttributeType;

using namespace helios::engine::rendering::mesh::types;
export namespace helios::opengl::OpenGLEnumMapper {

    /**
     * @brief Converts an engine primitive type to its OpenGL primitive enum.
     *
     * @param primitiveType Engine primitive topology.
     * @return Matching OpenGL topology enum, or `GL_TRIANGLES` as fallback.
     */
    [[nodiscard]] GLenum toOpenGL(const PrimitiveType primitiveType) noexcept {
        switch (primitiveType) {
            case PrimitiveType::Points:
                return GL_POINTS;
            case PrimitiveType::Lines:
                return GL_LINES;
            case PrimitiveType::LineLoop:
                return GL_LINE_LOOP;
            case PrimitiveType::LineStrip:
                return GL_LINE_STRIP;
            case PrimitiveType::Triangles:
                return GL_TRIANGLES;
            case PrimitiveType::TriangleStrip:
                return GL_TRIANGLE_STRIP;
            case PrimitiveType::TriangleFan:
                return GL_TRIANGLE_FAN;
            default:
                return GL_TRIANGLES;
        }
    }

    /**
     * @brief Converts an engine attribute type to OpenGL.
     *
     * @param attributeType Engine attribute type.
     * @return Matching OpenGL attribute type enum.
     */
    [[nodiscard]] GLenum toOpenGLBaseType(const VertexAttributeType attributeType) noexcept {
        switch (attributeType) {
            case VertexAttributeType::Float:
            case VertexAttributeType::Vec3f:
            case VertexAttributeType::Vec4f:
            case VertexAttributeType::Mat4f:
                return GL_FLOAT;
            default:
                std::unreachable();
        }
    }
}