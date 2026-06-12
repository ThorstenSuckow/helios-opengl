/**
 * @file OpenGLShaderCompileManager.ixx
 * @brief OpenGL manager that compiles shader sources into native OpenGL programs.
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


export module helios.opengl.OpenGLShaderCompileManager;

import helios.engine.util.log;
import helios.engine.util.io;
import helios.engine.runtime.world.tags;
import helios.engine.runtime.world;
import helios.engine.rendering.shader.commands;
import helios.engine.rendering.shader.components;
import helios.engine.rendering.shader.ShaderEntity;
import helios.engine.rendering.shader.types;
import helios.engine.rendering.shader.concepts;
import helios.engine.rendering.shader.commands;
import helios.engine.rendering.shader.NullUniformCacheStrategy;

import helios.opengl.components.OpenGLShaderComponent;

import helios.engine.runtime.world.EngineWorld;
import helios.engine.runtime.messaging.command.concepts;
import helios.engine.runtime.messaging.command.NullCommandBuffer;
import helios.engine.runtime.messaging.command.CommandHandlerRegistry;
import helios.engine.runtime.concepts;

using namespace helios::engine::runtime::world;
using namespace helios::engine::util::log;
using namespace helios::engine::util::io;
using namespace helios::engine::runtime::world::tags;
using namespace helios::engine::rendering::shader::commands;
using namespace helios::engine::rendering::shader::components;
using namespace helios::engine::rendering::shader;
using namespace helios::engine::rendering::shader::types;
using namespace helios::engine::rendering::shader::concepts;
using namespace helios::engine::rendering::shader::commands;
using namespace helios::opengl::components;
using namespace helios::engine::runtime::messaging::command::concepts;
using namespace helios::engine::runtime::messaging::command;
#define HELIOS_LOG_SCOPE "helios::opengl::OpenGLShaderCompileManager"
export namespace helios::opengl {

    /**
     * @brief Manager that consumes shader compile commands and performs OpenGL compilation/linking.
     *
     * @tparam THandle Shader handle type.
     * @tparam TUniformCacheStrategy Uniform cache strategy used after successful program linking.
     */
    template<typename THandle, typename TUniformCacheStrategy = NullUniformCacheStrategy<THandle>>
    requires IsShaderHandle<THandle> && IsUniformCacheStrategyLike<TUniformCacheStrategy, THandle, UniformScope::Pass, UniformScope::Draw>
    class OpenGLShaderCompileManager {

        /**
         * @brief File reader used to load shader source text from disk.
         */
        BasicStringFileReader stringFileReader_;

        /**
         * @brief Render-resource world used to resolve shader entities by handle.
         */
        RenderResourceWorld& renderResourceWorld_;

        /**
         * @brief Pending shader handles queued for compilation during `flush(...)`.
         */
        std::vector<ShaderHandle> shaderHandles_;

        /**
         * @brief Reused storage for loaded vertex shader source.
         */
        std::string vertexShaderSource_;

        /**
         * @brief Reused storage for loaded fragment shader source.
         */
        std::string fragmentShaderSource_;


        inline static const Logger& logger_ = LogManager::loggerForScope(HELIOS_LOG_SCOPE);

        /**
         * @brief Uniform caching strategy executed after successful shader compilation.
         */
        TUniformCacheStrategy uniformCacheStrategy_;

        /**
         * @brief Loads the specified vertex and fragment shader.
         *
         * @param vertexShaderPath Path to the vertex shader source file.
         * @param fragmentShaderPath Path to the fragment shader source file.
         * @param vertexShaderSource Output buffer receiving vertex shader source text.
         * @param fragmentShaderSource Output buffer receiving fragment shader source text.
         * @return true if loading succeeded, otherwise false.
         *
         * @throws if loading the specified files failed.
         */
        bool load(
            const std::string& vertexShaderPath,
            const std::string& fragmentShaderPath,
            std::string& vertexShaderSource,
            std::string& fragmentShaderSource
        ) noexcept {

            const bool frag = stringFileReader_.readInto(fragmentShaderPath, fragmentShaderSource);
            const bool vert = stringFileReader_.readInto(vertexShaderPath, vertexShaderSource);

            assert(frag && vert && "Could not load shader files");

            return frag && vert;
        }


        /**
         * @brief Compiles the vertex and fragment shader represented by this instance.
         *
         * @param shader Shader entity providing source and receiving OpenGL shader state.
         * @return true if compilation succeeded, otherwise false.
         *
         * @throws if compilation failed.
         */
        bool compile(ShaderEntity shader) noexcept {

            using Handle = typename ShaderEntity::Handle_type;

            logger_.info("Compiling shader...");

            if (shader.template get<OpenGLShaderComponent<Handle>>()) {
                logger_.error("Shader already has a ShaderComponent");
                assert(false && "Shader already has a ShaderComponent");
                return false;
            }

            auto* shaderSourceComponent = shader.template get<ShaderSourceComponent<Handle>>();


            const bool loaded = load(
                shaderSourceComponent->vertexShaderPath,
                shaderSourceComponent->fragmentShaderPath,
                vertexShaderSource_, fragmentShaderSource_);

            if (!loaded) {
                logger_.error("Could not load shader files");
                assert(false && "Could not load shader files");
                return false;
            }

            const GLchar* vertexSrc = vertexShaderSource_.c_str();
            const GLchar* fragmentSrc = fragmentShaderSource_.c_str();

            const unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
            glCompileShader(vertexShader);

            const unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
            glCompileShader(fragmentShader);

            int success;
            char infoLog[512];

            glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
                logger_.error("Vertex Shader Compilation failed.");
                assert(false && "Vertex Shader Compilation failed.");
                return false;
            }

            glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
                logger_.error("Fragment Shader Compilation failed.");
                assert(false && "Fragment Shader Compilation failed.");
                return false;
            }

            auto progId = glCreateProgram();

            glAttachShader(progId, vertexShader);
            glAttachShader(progId, fragmentShader);
            glLinkProgram(progId);

            glGetProgramiv(progId, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(progId, 512, nullptr, infoLog);
                logger_.error("Program linking failed.");
                assert(false && "Program linking failed.");
                return false;
            }

            auto& shaderComponent = shader.add<OpenGLShaderComponent<THandle>>();
            shaderComponent.programId = progId;

            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            vertexShaderSource_.clear();
            fragmentShaderSource_.clear();

            return true;
        }

        public:

        /**
         * @brief Constructs the manager with access to render-resource world storage.
         *
         * @param renderResourceWorld Render-resource world used to resolve shader entities.
         * @param uniformCacheStrategy Strategy object used to cache pass/draw uniforms.
         */
        explicit OpenGLShaderCompileManager(
            RenderResourceWorld& renderResourceWorld,
            TUniformCacheStrategy&& uniformCacheStrategy
        )
        : 
        renderResourceWorld_(renderResourceWorld),
        uniformCacheStrategy_(std::move(uniformCacheStrategy))
        { }

        /**
         * @brief Engine role marker used by runtime registries.
         */
        using EngineRoleTag = ManagerRole;


        /**
         * @brief Compiles all queued shaders and clears processed command data.
         *
         * @param updateContext Frame-local update context.
         */
        void flush(UpdateContext& updateContext)  noexcept {

            if (shaderHandles_.empty()) {
                return;
            }

            for (const auto& sourceHandle : shaderHandles_) {
                auto shaderEntity = renderResourceWorld_.findEntity<THandle>(sourceHandle);

                if (!shaderEntity) {
                    logger_.error("Could not find shader source");
                    assert(false && "Could not find shader source");
                    continue;
                }

                if (!compile(*shaderEntity)) {
                    logger_.error("Could not compile shader");
                } else {
                    shaderEntity->template remove<ShaderSourceComponent<THandle>>();
                    std::ignore = uniformCacheStrategy_.template cacheUniforms<UniformScope::Pass>(shaderEntity->handle(), renderResourceWorld_, updateContext);
                    std::ignore = uniformCacheStrategy_.template cacheUniforms<UniformScope::Draw>(shaderEntity->handle(), renderResourceWorld_, updateContext);
                }
            }

            shaderHandles_.clear();
        }

        /**
         * @brief Enqueues a batch of shader handles for compilation.
         *
         * @param command Compile command that is consumed by move.
         * @return `true` if the command was accepted.
         */
        bool submit(ShaderBatchCompileCommand<THandle>&& command)  noexcept {
            shaderHandles_.reserve(shaderHandles_.size() + command.shaderHandles.size());

            for (auto& shaderHandle : command.shaderHandles) {
                shaderHandles_.push_back(std::move(shaderHandle));
            }
            return true;
        }

        /**
         * @brief Enqueues a single shader handle for compilation.
         *
         * @param command Compile command that is consumed by move.
         * @return `true` if the command was accepted.
         */
        bool submit(ShaderCompileCommand<THandle>&& command)  noexcept {
            shaderHandles_.push_back(std::move(command.shaderHandle));
            return true;
        }

        /**
         * @brief Registers compile command handlers in the runtime world.
         *
         * @param commandHandlerRegistry Registry used for command-handler registration.
         */
        void init(helios::engine::runtime::messaging::command::CommandHandlerRegistry& commandHandlerRegistry) noexcept {
            commandHandlerRegistry.registerHandler<ShaderCompileCommand<THandle>>(*this);
            commandHandlerRegistry.registerHandler<ShaderBatchCompileCommand<THandle>>(*this);
        }
    };


}