/**
 * @file OpenGLMeshUploadManager.ixx
 * @brief OpenGL manager that uploads mesh data and creates VAO/VBO/EBO objects.
 */
module;

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <ranges>
#include <unordered_map>
#include <vector>


export module helios.opengl.OpenGLMeshUploadManager;

import helios.engine.util.log;
import helios.engine.util.io;

import helios.engine.rendering.common.types.Vertex;
import helios.engine.rendering.mesh.commands;
import helios.engine.rendering.mesh.components;
import helios.engine.rendering.mesh.MeshEntity;
import helios.engine.rendering.mesh.types;
import helios.engine.rendering.mesh.concepts;

import helios.engine.runtime.world.EngineWorld;
import helios.engine.runtime.messaging.command.concepts;
import helios.engine.runtime.messaging.command.NullCommandBuffer;
import helios.engine.runtime.messaging.command.CommandHandlerRegistry;
import helios.engine.runtime.concepts;
import helios.engine.runtime.world.tags;
import helios.engine.runtime.world;


import helios.opengl.components.OpenGLMeshComponent;
import helios.opengl.OpenGLEnumMapper;

using namespace helios::engine::runtime::world;
using namespace helios::engine::util::log;
using namespace helios::engine::util::io;
using namespace helios::engine::runtime::world::tags;
using namespace helios::engine::rendering::mesh::commands;
using namespace helios::engine::rendering::mesh::components;
using namespace helios::engine::rendering::mesh;
using namespace helios::engine::rendering::mesh::types;
using namespace helios::engine::rendering::mesh::concepts;
using namespace helios::engine::rendering::mesh::commands;
using namespace helios::engine::rendering::common::types;
using namespace helios::opengl;
using namespace helios::opengl::components;
using namespace helios::engine::runtime::messaging::command::concepts;
using namespace helios::engine::runtime::messaging::command;

#define HELIOS_LOG_SCOPE "helios::opengl::OpenGLMeshUploadManager"
export namespace helios::opengl {

    /**
     * @brief Manager that consumes mesh-upload commands and performs OpenGL buffer setup.
     *
     * @tparam THandle Mesh handle type.
     * @tparam TCommandBuffer Command buffer type (kept for compatibility with runtime wiring).
     */
    template<typename THandle, typename TCommandBuffer = NullCommandBuffer>
    requires IsMeshHandle<THandle> && IsCommandBufferLike<TCommandBuffer>
    class OpenGLMeshUploadManager {

        /**
         * @brief Render-resource world used to resolve mesh entities by handle.
         */
        RenderResourceWorld& renderResourceWorld_;

        /**
         * @brief Pending mesh handles queued for upload during `flush(...)`.
         */
        std::vector<MeshHandle> meshHandles_;


        inline static const Logger& logger_ = LogManager::loggerForScope(HELIOS_LOG_SCOPE);

        /**
         * @brief Derived OpenGL attribute layout information for one engine attribute type.
         */
        struct AttributeFormat {
            std::uint32_t locationCount;
            std::uint32_t componentsPerLocation;
            std::uint32_t locationStrideBytes;
        };

        /**
         * @brief Configures OpenGL vertex attribute pointers from one layout component.
         *
         * @tparam TVertexInputRate Input-rate tag (`PerVertex` or `PerInstance`).
         * @param layoutComponent Attribute layout component to translate to OpenGL calls.
         */
        template<typename TVertexInputRate>
        void buildVertexAttributeLayout(const VertexAttributeLayoutComponent<THandle, TVertexInputRate>* layoutComponent) noexcept {

            const auto& active = layoutComponent->active();
            const auto& layouts = layoutComponent->layouts();

            for (std::size_t idx = 0; idx < active.size(); idx++) {

                if (!active.test(idx)) {
                    continue;
                }

                const auto& layout = layouts[idx];

                const auto format = fromAttributeType(layout.attribute.type);

                for (std::uint32_t i = 0; i < format.locationCount; i++) {
                    const auto location = layout.location + i;

                    glEnableVertexAttribArray(location);
                    glVertexAttribPointer(
                        location,
                        format.componentsPerLocation,
                        OpenGLEnumMapper::toOpenGLBaseType(layout.attribute.type),
                        GL_FALSE,
                        static_cast<GLsizei>(layout.stride),
                        reinterpret_cast<void*>(layout.offset + (i * format.locationStrideBytes))
                    );
                    glVertexAttribDivisor(location, layout.divisor);
                }
            }

        }


        /**
         * @brief Maps engine attribute type to OpenGL attribute format details.
         *
         * @param attributeType Engine-side vertex attribute type.
         * @return Expanded format used to configure one or multiple attribute locations.
         */
        [[nodiscard]] AttributeFormat fromAttributeType(VertexAttributeType attributeType) const noexcept {

            switch (attributeType) {
                case VertexAttributeType::Float:
                    return AttributeFormat{1, 1, 0};
                case VertexAttributeType::Vec3f:
                    return AttributeFormat{1, 3, 0};
                case VertexAttributeType::Vec4f:
                    return AttributeFormat{1, 4, 0};
                case VertexAttributeType::Mat4f:
                    return AttributeFormat{4, 4, sizeof(float) * 4};
                default:
                    std::unreachable();
            }

        }

        /**
         * @brief Uploads one mesh to GPU buffers and configures vertex attributes.
         *
         * @param mesh Mesh entity containing upload request and mesh data.
         * @return `true` if upload succeeded, otherwise `false`.
         */
        bool upload(MeshEntity mesh) noexcept {

            using Handle = typename MeshEntity::Handle_type;

            logger_.info("Uploading mesh data for MeshEntity {0}...", mesh.handle().entityId);

            if (!mesh.template get<MeshUploadRequestComponent<Handle>>()) {
                logger_.error("MeshUpload not requested by this entity");
                assert(false && "MeshUpload not requested by this entity");
                return false;
            }


            if (mesh.template get<OpenGLMeshComponent<Handle>>()) {
                logger_.error("Mesh already has a MeshComponent");
                assert(false && "Mesh already has a MeshComponent");
                return false;
            }

            auto* meshDataComponent = mesh.template get<MeshDataComponent<Handle>>();
            auto& meshData = meshDataComponent->meshData;

            auto& openglMesh = mesh.template add<OpenGLMeshComponent<Handle>>();

            openglMesh.indexCount    = meshData.indices.size();
            openglMesh.primitiveType = OpenGLEnumMapper::toOpenGL(meshData.primitiveType);

            auto* vertexAttributeLayoutComponent = mesh.template get<VertexAttributeLayoutComponent<Handle, PerVertex>>();
            assert(vertexAttributeLayoutComponent && "Expected a VertexAttributeLayoutComponent for PerVertex attributes");
            auto* instancedAttributeLayoutComponent = mesh.template get<VertexAttributeLayoutComponent<Handle, PerInstance>>();

            glGenVertexArrays(1, &openglMesh.vao);
            glGenBuffers(1, &openglMesh.vbo);
            glGenBuffers(1, &openglMesh.ebo);

            if (instancedAttributeLayoutComponent) {
                glGenBuffers(1, &openglMesh.instanceVbo);
            }

            glBindVertexArray(openglMesh.vao);

            // vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, openglMesh.vbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                meshData.vertices.size() * sizeof(Vertex),
                &(meshData.vertices)[0],
                GL_STATIC_DRAW
            );

            // element buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openglMesh.ebo);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                meshData.indices.size() * sizeof(unsigned int),
                &(meshData.indices)[0],
                GL_STATIC_DRAW
            );

            // per instance buffer
            buildVertexAttributeLayout<PerVertex>(vertexAttributeLayoutComponent);
            mesh.template remove<VertexAttributeLayoutComponent<Handle, PerVertex>>();

            if (instancedAttributeLayoutComponent) {
                glBindBuffer(GL_ARRAY_BUFFER, openglMesh.instanceVbo);
                buildVertexAttributeLayout<PerInstance>(instancedAttributeLayoutComponent);
                mesh.template remove<VertexAttributeLayoutComponent<Handle, PerInstance>>();
            }


            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            return true;
        }

        public:

        /**
         * @brief Constructs the manager with access to render-resource storage.
         *
         * @param renderResourceWorld Render-resource world used to resolve mesh entities.
         */
        explicit OpenGLMeshUploadManager(RenderResourceWorld& renderResourceWorld)
        :
        renderResourceWorld_(renderResourceWorld)
        { }

        /**
         * @brief Engine role marker used by runtime registries.
         */
        using EngineRoleTag = ManagerRole;


        /**
         * @brief Uploads all queued meshes and clears processed queue entries.
         *
         * @param updateContext Frame-local update context.
         */
        void flush(UpdateContext& updateContext)  noexcept {

            if (meshHandles_.empty()) {
                return;
            }

            for (const auto& sourceHandle : meshHandles_) {
                auto meshEntity = renderResourceWorld_.findEntity<THandle>(sourceHandle);

                if (!meshEntity) {
                    logger_.error("Could not find mesh entity");
                    assert(false && "Could not find mesh entity");
                    continue;
                }

                if (!upload(*meshEntity)) {
                    logger_.error("Could not compile mesh");
                } else {
                    meshEntity->template remove<MeshUploadRequestComponent<THandle>>();
                }
            }

            meshHandles_.clear();
        }

        /**
         * @brief Queues mesh handles from a batch upload command.
         *
         * @param command Batch command containing mesh handles to upload (consumed by value).
         * @return `true` when the command was accepted.
         */
        bool submit(MeshBatchUploadCommand<THandle>&& command)  noexcept {
            meshHandles_.reserve(meshHandles_.size() + command.meshHandles.size());

            for (const auto& meshHandle : command.meshHandles) {
                meshHandles_.push_back(std::move(meshHandle));
            }
            return true;
        }

        /**
         * @brief Registers mesh upload command handlers in the command registry.
         *
         * @param commandHandlerRegistry Registry used for command-handler registration.
         */
        void init(helios::engine::runtime::messaging::command::CommandHandlerRegistry& commandHandlerRegistry) noexcept {
            commandHandlerRegistry.registerHandler<MeshBatchUploadCommand<THandle>>(*this);
        }
    };


}