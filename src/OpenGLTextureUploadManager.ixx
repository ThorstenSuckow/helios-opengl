
module;

#include <glad/gl.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>


export module helios.opengl.OpenGLTextureUploadManager;

import helios.engine.util.log;
import helios.engine.util.io;

import helios.engine.util.io;

import helios.engine.rendering.common.types.Vertex;
import helios.engine.rendering.texture.commands;
import helios.engine.rendering.texture.components;
import helios.engine.rendering.texture.TextureEntity;
import helios.engine.rendering.texture.types;
import helios.engine.rendering.texture.concepts;

import helios.engine.runtime.world.EngineWorld;
import helios.engine.runtime.messaging.command.concepts;
import helios.engine.runtime.messaging.command.NullCommandBuffer;
import helios.engine.runtime.messaging.command.CommandHandlerRegistry;
import helios.engine.runtime.concepts;
import helios.engine.runtime.world.tags;
import helios.engine.runtime.world;

import helios.opengl.components;

using namespace helios::engine::util::io;
using namespace helios::engine::runtime::world;
using namespace helios::engine::util::log;
using namespace helios::engine::util::io;
using namespace helios::engine::runtime::world::tags;

using namespace helios::engine::rendering;
using namespace helios::opengl::components;
using namespace helios::engine::runtime::messaging::command::concepts;
using namespace helios::engine::runtime::messaging::command;

#define HELIOS_LOG_SCOPE "helios::opengl::OpenGLTextureUploadManager"
export namespace helios::opengl {

    /**
     * @brief Manager that consumes mesh-upload commands and performs OpenGL buffer setup.
     *
     * @tparam THandle Mesh handle type.
     * @tparam TCommandBuffer Command buffer type (kept for compatibility with runtime wiring).
     */
    template<typename THandle = texture::types::TextureHandle>
    requires texture::concepts::IsTextureHandle<THandle>
    class OpenGLTextureUploadManager {

        /**
         * @brief Render-resource world used to resolve texture entities by handle.
         */
        RenderResourceWorld& renderResourceWorld_;

        /**
         * @brief Pending mesh handles queued for upload during `flush(...)`.
         */
        std::vector<THandle> textureHandles_;


        inline static const Logger& logger_ = LogManager::loggerForScope(HELIOS_LOG_SCOPE);


        /**
         * @brief Uploads one texture to GPU buffers.
         *
         * @param texture Texture entity containing upload request and texture data.
         *
         * @return `true` if upload succeeded, otherwise `false`.
         */
        bool upload(texture::TextureEntity texture) noexcept requires std::same_as<THandle, typename texture::TextureEntity::Handle_type> {


            logger_.info("Uploading texture data for MeshEntity {0}...", texture.handle().entityId);


            auto* textureSourceCmp = texture.template get<texture::components::TextureSourceComponent<THandle>>();
            if (!textureSourceCmp) {
                logger_.error("Texture upload not requested by this entity");
                assert(false && "Texture upload not requested by this entity");
                return false;
            }


            if (texture.template get<OpenGLTextureComponent<THandle>>()) {
                logger_.error("Mesh already has a MeshComponent");
                assert(false && "Mesh already has a MeshComponent");
                return false;
            }

            const auto imageData = imageReader_.data(textureSourceCmp->texturePath);

            unsigned int textureId;

            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            assert(imageData.nrChannels == 4 && "Expected 4 channels (RGBA) in texture data");

            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA,
                imageData.width, imageData.height,
                0, GL_RGBA, GL_UNSIGNED_BYTE,
                imageData.data.data()
            );
            glGenerateMipmap(GL_TEXTURE_2D);

            texture.template add<OpenGLTextureComponent<THandle>>(textureId);

            return true;
        }

        const ImageReader& imageReader_;

        public:

        /**
         * @brief Constructs the manager with access to render-resource storage.
         *
         * @param renderResourceWorld Render-resource world used to resolve mesh entities.
         * @param imageReader Image reader used to load texture data from disk.
         */
        explicit OpenGLTextureUploadManager(RenderResourceWorld& renderResourceWorld, const ImageReader &imageReader)
        :
        renderResourceWorld_(renderResourceWorld),
        imageReader_(imageReader)
        { }

        /**
         * @brief Engine role marker used by runtime registries.
         */
        using EngineRoleTag = ManagerRole;


        /**
         * @brief Uploads all queued textures and clears processed texture entries.
         *
         * @param updateContext Frame-local update context.
         */
        void flush(UpdateContext& updateContext)  noexcept {

            if (textureHandles_.empty()) {
                return;
            }

            for (const auto& sourceHandle : textureHandles_) {
                auto textureEntity = renderResourceWorld_.findEntity<THandle>(sourceHandle);

                if (!textureEntity) {
                    logger_.error("Could not find texture entity");
                    assert(false && "Could not find texture entity");
                    continue;
                }

                if (!upload(*textureEntity)) {
                    logger_.error("Could not upload texture");
                    assert(false && "Could not upload texture");
                } else {
                    textureEntity->template remove<texture::components::TextureSourceComponent<THandle>>();
                }
            }

            textureHandles_.clear();
        }

        /**
         * @brief Queues texture handles from a batch upload command.
         *
         * @param command Batch command containing texture handles to upload (consumed by value).
         * @return `true` when the command was accepted.
         */
        bool submit(texture::commands::TextureBatchUploadCommand<THandle>&& command)  noexcept {
            textureHandles_.reserve(textureHandles_.size() + command.textureHandles.size());

            for (const auto& textureHandle : command.textureHandles) {
                textureHandles_.push_back(std::move(textureHandle));
            }
            return true;
        }

        /**
         * @brief Registers texture upload command handlers in the command registry.
         *
         * @param commandHandlerRegistry Registry used for command-handler registration.
         */
        void init(
            helios::engine::runtime::messaging::command::CommandHandlerRegistry& commandHandlerRegistry
            ) noexcept {
            commandHandlerRegistry.registerHandler<texture::commands::TextureBatchUploadCommand<THandle>>(*this);
        }

    };


}