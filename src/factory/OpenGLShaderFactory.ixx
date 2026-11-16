module;

#include <string>

export module helios.opengl.factory.OpenGLShaderFactory;

import helios.ecs.entity;
import helios.opengl.components;
export namespace helios::opengl::factory {

    template<typename TRenderHandles>
    class OpenGLShaderFactory {

    using ShaderHandle = typename TRenderHandles::ShaderHandle;

    using EntityManager = ecs::entity::EntityManager<ShaderHandle>;
    using ShaderEntity = ecs::entity::Entity<EntityManager>;

public:

    explicit OpenGLShaderFactory() = default;

    ShaderEntity createShader(
        ecs::entity::EntityWorld& world,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    ) {
        auto shaderEntity = world.add<ShaderHandle>();
        shaderEntity.template add<components::OpenGLShaderComponent>(vertexShaderPath, fragmentShaderPath);
        return shaderEntity;
    }
};
}