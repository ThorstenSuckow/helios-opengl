
module;

#include <glad/gl.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>


export module helios.opengl.OpenGLTextureUploadManager;

import helios.core.log;
import helios.core.io;

import helios.engine.rendering.common.types.Vertex;
import helios.engine.rendering.texture.commands;
import helios.engine.rendering.texture.components;

import helios.ecs;


import helios.opengl.components;

using namespace helios::core::io;
using namespace helios::core::log;
using namespace helios::core::io;


using namespace helios::engine::rendering;
using namespace helios::opengl::components;
using namespace helios::ecs::common::concepts;
using namespace helios::ecs;

#define HELIOS_LOG_SCOPE "helios::opengl::OpenGLTextureUploadManager"
export namespace helios::opengl {

    /**
     * @brief Manager that consumes mesh-upload commands and performs OpenGL buffer setup.
     *
     * @tparam THandle Mesh handle type.
     * @tparam TCommandBuffer Command buffer type (kept for compatibility with runtime wiring).
     */
    template<typename THandle>
    class OpenGLTextureUploadManager {

        using TextureEntity = ecs::entity::Entity<ecs::entity::EntityManager<THandle>>;
        
        
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
        bool upload(TextureEntity texture, ImageReader& imageReader) noexcept
        requires std::same_as<THandle, typename TextureEntity::HandleType> {


            logger_.info("Uploading texture data for MeshEntity {0}...", texture.handle().entityId());


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

            ImageData imageData{};
            logger_.info("Uploading {0}", textureSourceCmp->texturePath);
            if (!imageReader.readInto(textureSourceCmp->texturePath, imageData)) {
                assert(false && "Texture upload failed");
                logger_.error("Texture upload failed.");
                return false;
            }
            logger_.info("... done uploading.");

            unsigned int textureId;

            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);

            /**
             * @todo when bound, this can be changed during runtime, i.e. move to render backend
             */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            if (imageData.nrChannels != 4) {
                assert(false && "Expected 4 channels (RGBA) in texture data");
                logger_.error("Expected 4 channels (RGBA) in texture data");
                return false;
            }

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

        public:


        /**
         * @brief Uploads all queued textures and clears processed texture entries.
         *
         * @param updateContext Frame-local update context.
         */
        bool commit(entity::EntityManager<THandle>& entityManager)  noexcept {

            if (textureHandles_.empty()) {
                return true;
            }

            ImageReader imageReader{};

            for (const auto& sourceHandle : textureHandles_) {
                auto textureEntity = entityManager.entity(sourceHandle);

                if (!textureEntity) {
                    logger_.error("Could not find texture entity");
                    assert(false && "Could not find texture entity");
                    continue;
                }

                /**
                 * @todo fallback texture?
                 */
                if (!upload(*textureEntity, imageReader)) {
                    logger_.error("Could not upload texture");
                    assert(false && "Could not upload texture");
                } else {
                    textureEntity->template remove<texture::components::TextureSourceComponent<THandle>>();
                }
            }

            textureHandles_.clear();
            return true;
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
        bool init(ecs::command::CommandHandlerRegistry& commandHandlerRegistry) noexcept {
            commandHandlerRegistry.template registerHandler<texture::commands::TextureBatchUploadCommand<THandle>>(*this);
            return true;
        }

        void reset() {
            /*intentionally noop*/
        }
    };


}