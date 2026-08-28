/**
 * @file OpenGLRenderDataResolver.ixx
 * @brief Concrete resolver for consuming helios ecs handles as  low-level data for OpenGL draw calls.
 */
module;

#include <glad/gl.h>
#include <utility>
#include <optional>
#include <cassert>
#include <typeinfo>

export module helios.opengl.OpenGLRenderDataResolver;

import helios.ecs;

import helios.engine.scene.components;

import helios.engine.spatial.components;
import helios.engine.rendering.common.components;
import helios.engine.rendering.shader.types;
import helios.engine.core.components;

import helios.opengl.types;
import helios.opengl.components;
import helios.core.log;

#define HELIOS_LOG_SCOPE "helios::opengl::OpenGLRenderDataResolver"
export namespace helios::opengl {


    template<typename THandleList>
    class OpenGLRenderDataResolver {

        template<typename THandle>
        using EntityManager = ecs::EntityManager<THandle>;

        inline static const core::log::Logger& logger_ = core::log::LogManager::loggerForScope(
            HELIOS_LOG_SCOPE
        );

        template<typename THandle, typename TEntity>
        types::OpenGLClearColorData clearColorData(TEntity& entity) noexcept {

            types::OpenGLClearColorData clearColorData{};

            auto* colorComp = entity->template get<engine::core::components::ColorComponent<THandle>>();
            auto* clearComp = entity->template get<engine::rendering::common::components::ClearComponent<THandle>>();

            if (colorComp) {
                const auto clearColor = colorComp->value();
                clearColorData.color = {clearColor[0], clearColor[1], clearColor[2], clearColor[3]};
            }

            if (clearComp) {
                const auto clearFlags = std::to_underlying(clearComp->flags);
                clearColorData.clearFlags = clearFlags;
            }

            return clearColorData;
        }

        template<typename TEntity>
        [[nodiscard]] std::optional<types::OpenGLViewProjectionData> viewProjection(const TEntity& viewportEntity, ecs::EcsWorld& ecsWorld) const noexcept {
            auto* cbc = viewportEntity.template get<engine::scene::components::CameraBindingComponent<typename TEntity::HandleType>>();
            if (!cbc) {
                logger_.error("Expected CameraBindingComponent on ViewportEntity, but couldn't find any.");
                return std::nullopt;
            }
            auto camera = ecsWorld.find(cbc->targetHandle());
            if (!camera) {
                logger_.error("Expected CameraEntity, but couldn't find any.");
                return std::nullopt;
            }
            using CameraHandleType = std::remove_cvref_t<decltype(cbc->targetHandle())>;
            auto* vm = camera->template get<engine::scene::components::ViewMatrixComponent<CameraHandleType>>();
            if (!vm) {
                logger_.error("Expected ViewMatrixComponent, but couldn't find any.");
                return std::nullopt;
            }

            auto* pm = camera->template get<engine::scene::components::ProjectionMatrixComponent<CameraHandleType>>();
            if (!pm) {
                logger_.error("Expected ProjectionMatrixComponent, but couldn't find any.");
                return std::nullopt;
            }

            return types::OpenGLViewProjectionData{
                vm->value(), pm->value()
            };
        }

    public:

        using RenderTargetHandleType = typename THandleList::RenderTargetHandleType;
        using ViewportHandleType = typename THandleList::ViewportHandleType;
        using ShaderHandleType = typename THandleList::ShaderHandleType;
        using TextureHandleType = typename THandleList::TextureHandleType;
        using MaterialHandleType = typename THandleList::MaterialHandleType;
        using MeshHandleType = typename THandleList::MeshHandleType;



        [[nodiscard]] std::optional<types::OpenGLRenderTargetData> resolveRenderTargetData(
            EntityManager<RenderTargetHandleType>& entityManager, const RenderTargetHandleType renderTargetHandle) noexcept {

            types::OpenGLRenderTargetData renderTargetData{};

            auto renderTargetEntity = entityManager.entity(renderTargetHandle);

            if (!renderTargetEntity) {
                logger_.error("Missing RenderTargetEntity for handle {0}.", renderTargetHandle.entityId());
                assert(renderTargetEntity && "Missing RenderTargetEntity for handle.");
                return std::nullopt;
            }

            auto renderTargetSize = renderTargetEntity->template get<engine::spatial::components::Size2DComponent<RenderTargetHandleType>>()->value();
            const auto renderTargetId = renderTargetEntity->template get<components::OpenGLRenderTargetIdComponent<RenderTargetHandleType>>()->value();

            renderTargetData.renderTargetId = renderTargetId;
            renderTargetData.renderTargetSize = {renderTargetSize[0], renderTargetSize[1]};
            renderTargetData.clearColorData = clearColorData<RenderTargetHandleType>(renderTargetEntity);

            return renderTargetData;
        }

        [[nodiscard]] std::optional<types::OpenGLViewportData> resolveViewportData(
            const ViewportHandleType viewportHandle, ecs::EcsWorld& ecsWorld) noexcept {

            types::OpenGLViewportData viewportData{};

            auto viewport = ecsWorld.find<ViewportHandleType>(viewportHandle);

            if (!viewport) {
                logger_.error("Missing Viewport for handle {0}.", viewportHandle.entityId());
                assert(viewport && "Missing Viewport for handle.");
                return std::nullopt;
            }

            viewportData.viewProjectionData = viewProjection(*viewport, ecsWorld);

            auto bounds = viewport->template get<engine::spatial::components::RectComponent<ViewportHandleType>>()->value();
            viewportData.viewportBounds  = {bounds[0], bounds[1], bounds[2], bounds[3]};

            viewportData.clearColorData = clearColorData<ViewportHandleType>(viewport);

            return viewportData;
        }

        [[nodiscard]] std::optional<types::OpenGLShaderData> resolveShaderData(
            const ShaderHandleType shaderHandle, EntityManager<ShaderHandleType>& entityManager) noexcept {

            types::OpenGLShaderData shaderData{};

            auto shaderEntity = entityManager.entity(shaderHandle);
            if (!shaderEntity) {
                logger_.error("ShaderEntity expected, but not found");
                assert(false && "ShaderEntity not found");
                return std::nullopt;
            }

            auto* openglShader = shaderEntity->template get<components::OpenGLShaderComponent<ShaderHandleType>>();
            if (!openglShader) {
                logger_.error("OpenGLShader expected, but not found");
                assert(false && "OpenGLShader not found");
                return std::nullopt;
            }

            shaderData.programId = openglShader->programId;

            using UniformScope =engine::rendering::shader::types::UniformScope::Pass;

            auto* ulc = shaderEntity->template get<components::OpenGLUniformWriteOperationsComponent<
                ShaderHandleType, UniformScope>>();

            if (!ulc) {
                logger_.error("OpenGLUniformWriteOperationsComponent<{0}> expected, but not found", typeid(UniformScope).name());
                assert(false && "OpenGLUniformWriteOperationsComponent not found");
                return std::nullopt;
            }

            shaderData.uniformWriteOperations = ulc->operations;

            return shaderData;
        }

        [[nodiscard]] std::optional<types::OpenGLTextureData> resolveTextureData(
            const TextureHandleType textureHandle, EntityManager<TextureHandleType>& entityManager) noexcept {

            types::OpenGLTextureData textureData{};

            auto textureEntity = entityManager.entity(textureHandle);
            if (!textureEntity) {
                logger_.error("TextureEntity expected, but not found");
                assert(false && "TextureEntity not found");
                return std::nullopt;
            }

            auto* openglTexture = textureEntity->template get<components::OpenGLTextureComponent<TextureHandleType>>();
            if (!openglTexture) {
                logger_.error("OpenGLTexture expected, but not found");
                assert(false && "OpenGLTexture not found");
                return std::nullopt;
            }

            textureData.textureId = openglTexture->textureId;

            return textureData;
        }


        [[nodiscard]] std::optional<types::OpenGLMaterialData> resolveMaterialData(
            const MaterialHandleType materialHandle, EntityManager<MaterialHandleType>& entityManager) noexcept {

            types::OpenGLMaterialData materialData{};

            auto materialEntity = entityManager.entity(materialHandle);
            if (!materialEntity) {
                logger_.error("MaterialEntity expected, but not found");
                assert(false && "MaterialEntity not found");
                return std::nullopt;
            }

            if (auto* colorComponent = materialEntity->template get<engine::core::components::ColorComponent<MaterialHandleType>>()) {
                materialData.baseColor = colorComponent->value();
            }

            return materialData;
        }


        [[nodiscard]] std::optional<types::OpenGLMeshData> resolveMeshData(
            const MeshHandleType meshHandle, EntityManager<MeshHandleType>& entityManager) noexcept {

            types::OpenGLMeshData meshData{};

            auto meshEntity = entityManager.entity(meshHandle);
            if (!meshEntity) {
                logger_.error("MeshEntity expected, but not found");
                assert(false && "MeshEntity not found");
                return std::nullopt;
            }

            auto* openglMesh = meshEntity->template get<components::OpenGLMeshComponent<MeshHandleType>>();
            if (!openglMesh) {
                logger_.error("OpenGLMesh expected, but not found");
                assert(false && "OpenGLMesh not found");
                return std::nullopt;
            }

            meshData = openglMesh->data;

            return meshData;
        }




    };
}
