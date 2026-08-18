
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
import helios.engine.rendering.texture.TextureEntity;
import helios.engine.rendering.texture.types;
import helios.engine.rendering.texture.concepts;

import helios.ecs;
import helios.engine.runtime.concepts;

import helios.engine.runtime.world;

import helios.opengl.components;

using namespace helios::core::io;
using namespace helios::engine::runtime::world;
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
    template<typename TInitContext, typename TExecutionContext, typename THandle = texture::types::TextureHandle>
    requires texture::concepts::IsTextureHandle<THandle> &&
            engine::runtime::concepts::ProvidesUpdateContext<TExecutionContext, engine::runtime::world::UpdateContext> &&
            ecs::common::concepts::ProvidesCommandHandlerRegistry<TInitContext, ecs::command::CommandHandlerRegistry>
    class OpenGLTextureUploadManager {

        /**
         * @brief Render-resource world used to resolve texture entities by handle.
         */
        EcsWorld& ecsWorld_;

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
            if (!imageReader_.readInto(textureSourceCmp->texturePath, imageData)) {
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

        const ImageReader imageReader_;

        public:

        using EcsRoleTag = ecs::manager::tags::ManagerRole;
        using InitContextType = TInitContext;
        using ExecutionContextType = TExecutionContext;


        /**
         * @brief Constructs the manager with access to render-resource storage.
         *
         * @param ecsWorld Render-resource world used to resolve mesh entities.
         * @param imageReader Image reader used to load texture data from disk.
         */
        explicit OpenGLTextureUploadManager(EcsWorld& ecsWorld, const ImageReader &imageReader)
        :
        ecsWorld_(ecsWorld),
        imageReader_(imageReader)
        { }

        /**
         * @brief Uploads all queued textures and clears processed texture entries.
         *
         * @param updateContext Frame-local update context.
         */
        bool executeCommands(TExecutionContext&)  noexcept {

            if (textureHandles_.empty()) {
                return true;
            }

            for (const auto& sourceHandle : textureHandles_) {
                auto textureEntity = ecsWorld_.find<THandle>(sourceHandle);

                if (!textureEntity) {
                    logger_.error("Could not find texture entity");
                    assert(false && "Could not find texture entity");
                    continue;
                }

                /**
                 * @todo fallback texture?
                 */
                if (!upload(*textureEntity)) {
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
        bool init(TInitContext& initContext) noexcept {
            auto& commandHandlerRegistry = initContext.commandHandlerRegistry();


            commandHandlerRegistry.template registerHandler<texture::commands::TextureBatchUploadCommand<THandle>>(*this);
            return true;
        }

        void reset() {
            /*intentionally noop*/
        }
    };


}