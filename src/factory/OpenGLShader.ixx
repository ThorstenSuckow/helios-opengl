module;

#include <vector>
#include <string>

export module helios.opengl.factory.OpenGLShader;

import helios.ecs.entity;

import helios.engine.rendering.shader;

export namespace helios::opengl::factory {

    template<typename TRenderHandles>
    class OpenGLShader {


        using ShaderHandle = typename TRenderHandles::ShaderHandle;

        using EntityManager = helios::ecs::entity::EntityManager<ShaderHandle>;
        using ShaderEntity = helios::ecs::entity::Entity<EntityManager>;

        template<typename TUniformScope>
        using UniformMappingsComponent = helios::engine::rendering::shader::components::UniformMappingsComponent<TUniformScope>;

        std::vector<UniformMappingsComponent<helios::engine::rendering::shader::types::UniformScope::Pass>> uniformMappingsPass_;
        std::vector<UniformMappingsComponent<helios::engine::rendering::shader::types::UniformScope::Material>> uniformMappingsMaterial_;
        std::vector<UniformMappingsComponent<helios::engine::rendering::shader::types::UniformScope::Draw>> uniformMappingsDraw_;


        std::string vertexShaderPath_;
        std::string fragmentShaderPath_;

        std::string vertexShaderSource_;
        std::string fragmentShaderSource_;



    public:

        explicit OpenGLShader() = default;

        static OpenGLShader withSourcePath(std::string vertexShaderPath, std::string fragmentShaderPath) {
            OpenGLShader shader{};
            shader.vertexShaderPath_ = std::move(vertexShaderPath);
            shader.fragmentShaderPath_ = std::move(fragmentShaderPath);
            return shader;
        }

        static OpenGLShader withSourceCode(std::string vertexShaderSource, std::string fragmentShaderSource) {
            OpenGLShader shader{};
            shader.vertexShaderSource_ = std::move(vertexShaderSource);
            shader.fragmentShaderSource_ = std::move(fragmentShaderSource);
            return shader;
        }

        template<typename TUniformScope>
        OpenGLShader& addUniformMapping(UniformMappingsComponent<TUniformScope> mapping) {
            if constexpr (std::is_same_v<TUniformScope, helios::engine::rendering::shader::types::UniformScope::Pass>) {
                uniformMappingsPass_.push_back(std::move(mapping));
            } else if constexpr (std::is_same_v<TUniformScope, helios::engine::rendering::shader::types::UniformScope::Material>) {
                uniformMappingsMaterial_.push_back(std::move(mapping));
            } else if constexpr (std::is_same_v<TUniformScope, helios::engine::rendering::shader::types::UniformScope::Draw>) {
                uniformMappingsDraw_.push_back(std::move(mapping));
            }
            return *this;
        }




    };
}