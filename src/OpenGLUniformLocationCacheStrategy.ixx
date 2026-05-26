/**
 * @file OpenGLUniformLocationCacheStrategy.ixx
 * @brief Strategy that resolves and caches OpenGL uniform locations for shaders.
 */
module;

#include <glad/gl.h>
#include <vector>
#include <cassert>

export module helios.opengl.OpenGLUniformLocationCacheStrategy;

import helios.engine.rendering.shader.components.UniformMappingsComponent;
import helios.engine.runtime.world.EngineWorld;
import helios.engine.runtime.world.UpdateContext;
import helios.engine.rendering.shader.concepts.IsShaderHandle;

import helios.engine.util.log;

import helios.opengl.components.OpenGLUniformLocationComponent;
import helios.opengl.components.OpenGLShaderComponent;

using namespace helios::engine::runtime::world;
using namespace helios::engine::rendering::shader::concepts;
using namespace helios::engine::rendering::shader::components;
using namespace helios::engine::util::log;
using namespace helios::opengl::components;

#define HELIOS_LOG_SCOPE "helios::opengl::OpenGLUniformLocationCacheStrategy"
export namespace helios::opengl {

    /**
     * @brief Caches resolved OpenGL uniform locations for one shader entity.
     * @tparam THandle Shader handle type.
     */
    template<typename THandle>
    requires IsShaderHandle<THandle>
    class OpenGLUniformLocationCacheStrategy {

        static inline const Logger& logger_ = LogManager::loggerForScope(HELIOS_LOG_SCOPE);

    public:


        /**
         * @brief Resolves configured uniform names and stores their OpenGL locations.
         * @param entityHandle Shader entity handle.
         * @param renderResourceWorld Render-resource world containing shader entities.
         * @param updateContext Frame-local update context.
         * @return `true` on successful cache pass, otherwise `false`.
         * @details Reads `UniformMappingsComponent`, queries locations via `glGetUniformLocation`,
         * writes results into `OpenGLUniformLocationComponent`, and removes the mapping
         * component after processing.
         */
        template<typename TUniformScope>
        [[nodiscard]] bool cacheUniforms(
            THandle entityHandle,
            RenderResourceWorld& renderResourceWorld,
            UpdateContext& updateContext
        ) {
            auto shaderEntity = renderResourceWorld.findEntity(entityHandle);

            if (!shaderEntity) {
                logger_.error("ShaderEntity not found.");
                assert(shaderEntity && "ShaderEntity not found.");
                return false;
            }

            auto* umc = shaderEntity->template get<UniformMappingsComponent<THandle, TUniformScope>>();
            if (!umc) {
                logger_.info("No UniformMappingsComponent available.");
                return false;
            }

            auto* osc = shaderEntity->template get<OpenGLShaderComponent<THandle>>();
            if (!osc) {
                logger_.info("No OpenGLShaderComponent available.");
                return false;
            }

            GLuint programId = osc->programId;

            auto& ulc = shaderEntity->template getOrAdd<OpenGLUniformLocationComponent<THandle, TUniformScope>>();

            for (std::size_t i = 0; i < umc->mappings.size(); ++i ) {
                if (!umc->mappings[i].empty()) {
                    GLint location = glGetUniformLocation(programId, umc->mappings[i].c_str());
                    if (location != -1) {
                        ulc.locations[i] =  location;
                        logger_.warn("Assigning uniform for {0} to {1}", umc->mappings[i], location);
                    } else {
                        logger_.warn("Expected uniform for {0}, but found nothing", umc->mappings[i]);
                    }

                }
            }

            shaderEntity->template remove<UniformMappingsComponent<THandle, TUniformScope>>();

            return true;
        }

    };
}