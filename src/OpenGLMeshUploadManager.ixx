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

import helios.core.log;
import helios.core.io;

import helios.engine.rendering.common.types.Vertex;
import helios.engine.rendering.mesh.commands;
import helios.engine.rendering.mesh.components;
import helios.engine.rendering.mesh.types;

import helios.ecs;



import helios.opengl.components.OpenGLMeshComponent;
import helios.opengl.OpenGLEnumMapper;

using namespace helios::core::log;
using namespace helios::core::io;

using namespace helios::engine::rendering::mesh::commands;
using namespace helios::engine::rendering::mesh::components;
using namespace helios::engine::rendering::mesh;
using namespace helios::engine::rendering::mesh::types;
using namespace helios::engine::rendering::mesh::commands;
using namespace helios::engine::rendering::common::types;
using namespace helios::opengl;
using namespace helios::opengl::components;
using namespace helios::ecs::common::concepts;
using namespace helios::ecs;

#define HELIOS_LOG_SCOPE "helios::opengl::OpenGLMeshUploadManager"
export namespace helios::opengl {

    /**
     * @brief Manager that consumes mesh-upload commands and performs OpenGL buffer setup.
     *
     * @tparam THandle Mesh handle type.
     */
    template<typename THandle>
    class OpenGLMeshUploadManager {

        using MeshEntity = ecs::entity::Entity<ecs::entity::EntityManager<THandle>>;

        /**
         * @brief Pending mesh handles queued for upload during `flush(...)`.
         */
        std::vector<THandle> meshHandles_;


        inline static const Logger& logger_ = LogManager::loggerForScope(HELIOS_LOG_SCOPE);


        [[nodiscard]] bool usesInstancing(MeshEntity mesh) {
            auto* vertexAttributeLayoutsComponent = mesh.template get<VertexAttributeLayoutsComponent>();

            if (!vertexAttributeLayoutsComponent) {
                return false;
            }

            auto& layouts = vertexAttributeLayoutsComponent->layouts;

            for (auto i = 0; i < layouts.active().size(); i++) {
                if (!layouts.active().test(i)) {
                    continue;
                }
                if (layouts.items()[i].inputRate == VertexInputRate::PerInstance) {
                    return true;
                }
            }
            return false;
        }


        /**
         * @brief Uploads one mesh to GPU buffers and configures vertex attributes.
         *
         * @param mesh Mesh entity containing upload request and mesh data.
         * @return `true` if upload succeeded, otherwise `false`.
         */
        bool upload(MeshEntity mesh) noexcept {

            using Handle = typename MeshEntity::HandleType;

            logger_.info("Uploading mesh data for MeshEntity {0}...", mesh.handle().entityId());

            if (!mesh.template get<MeshUploadRequestComponent>()) {
                logger_.error("MeshUpload not requested by this entity");
                assert(false && "MeshUpload not requested by this entity");
                return false;
            }


            if (mesh.template get<OpenGLMeshComponent>()) {
                logger_.error("Mesh already has a MeshComponent");
                assert(false && "Mesh already has a MeshComponent");
                return false;
            }

            auto* meshDataComponent = mesh.template get<MeshDataComponent>();
            auto& meshData = meshDataComponent->meshData;

            auto& openglMesh = mesh.template add<OpenGLMeshComponent>();

            openglMesh.data.indexCount    = static_cast<uint32_t>(meshData.indices.size());
            openglMesh.data.primitiveType = OpenGLEnumMapper::toOpenGL(meshData.primitiveType);

            glGenVertexArrays(1, &openglMesh.data.vao);
            glGenBuffers(1, &openglMesh.data.vbo);
            glGenBuffers(1, &openglMesh.data.ebo);

            if (usesInstancing(mesh)) {
                glGenBuffers(1, &openglMesh.data.instanceVbo);
            }

            glBindVertexArray(openglMesh.data.vao);

            // vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, openglMesh.data.vbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                meshData.vertices.size() * sizeof(Vertex),
                &(meshData.vertices)[0],
                GL_STATIC_DRAW
            );

            // element buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openglMesh.data.ebo);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                meshData.indices.size() * sizeof(unsigned int),
                &(meshData.indices)[0],
                GL_STATIC_DRAW
            );

            mesh.template remove<MeshDataComponent>();


            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            return true;
        }

        public:


        /**
         * @brief Uploads all queued meshes and clears processed queue entries.
         *
         * @param updateContext Frame-local update context.
         */
        bool execute(entity::EntityManager<THandle>& entityManager)  noexcept {

            if (meshHandles_.empty()) {
                return true;
            }

            for (const auto& sourceHandle : meshHandles_) {
                auto meshEntity = entityManager.entity(sourceHandle);

                if (!meshEntity) {
                    logger_.error("Could not find mesh entity");
                    assert(false && "Could not find mesh entity");
                    continue;
                }

                if (!upload(*meshEntity)) {
                    logger_.error("Could not compile mesh");
                    return false;
                }

                meshEntity->template remove<MeshUploadRequestComponent>();

            }

            meshHandles_.clear();

            return true;
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
         * @param managerRegistry
         */
        bool init(ecs::command::CommandHandlerRegistry& commandHandlerRegistry) noexcept {

            commandHandlerRegistry.template registerHandler<MeshBatchUploadCommand<THandle>>(*this);
            return true;
        }

        void reset() {
            /*intentionally left noop*/
        }

    };


}