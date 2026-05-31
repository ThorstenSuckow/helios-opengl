# helios::opengl

OpenGL backend integration for the helios engine modules.

## Overview

`helios::opengl` connects engine-level rendering resources to OpenGL objects and
state changes. It provides backend execution, shader compilation, mesh upload,
uniform-location caching, and typed uniform writes.

## Features

- OpenGL render backend for render-target and viewport execution
- Shader compilation/linking manager for engine shader resources
- Mesh upload manager for VAO/VBO/EBO creation
- Uniform-location caching strategy
- Typed uniform writer for view, projection, model, and material uniforms
- OpenGL-specific ECS components for shader, mesh, uniform, and render-target data

## Module surface

| Area | Public modules / APIs |
|------|------------------------|
| Backend | `OpenGLBackend` |
| Resource managers | `OpenGLShaderCompileManager`, `OpenGLMeshUploadManager` |
| Uniforms | `OpenGLUniformLocationCacheStrategy`, `OpenGLUniformWriter` |
| Components | `helios.opengl.components` |
| Types | `helios.opengl.types`, `OpenGLEnumMapper` |

## Usage

### C++ module

```cpp
import helios.opengl;
```

### Backend architecture

`OpenGLBackend` translates engine render data into OpenGL state changes and draw
calls. It resolves viewport, camera, render-target, material, mesh, and shader
components from engine worlds.

`OpenGLShaderCompileManager<THandle, TUniformCacheStrategy>` consumes shader
compile commands and attaches `OpenGLShaderComponent` data to shader entities.

`OpenGLMeshUploadManager<THandle>` consumes mesh upload commands and creates the
OpenGL VAO/VBO/EBO state stored in `OpenGLMeshComponent`.

### CMake

Build and install:

```bash
cmake -S . -B build -DHELIOS_OPENGL_ENABLE_PACKAGE=ON -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build
cmake --install build
```

Consume from another project:

```cmake
find_package(helios-engine CONFIG REQUIRED)
find_package(helios-opengl CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE helios::opengl)
```

Configure a consumer against an installed prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/helios-prefix"
cmake --build build
```

## Development

Run the regular CMake build from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

## Related repositories

- [`helios-ecs`](https://github.com/thorstensuckow/helios-ecs)
- [`helios-engine`](https://github.com/thorstensuckow/helios-engine)
- [`helios-math`](https://github.com/thorstensuckow/helios-math)
- [`helios-glfw`](https://github.com/thorstensuckow/helios-glfw)
