cmake_minimum_required(VERSION 4.0)
project(helios-opengl LANGUAGES CXX)

include(FetchContent)

set(HELIOS_MATH_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-math" CACHE PATH "Local path to helios-math repository")
set(HELIOS_ENGINE_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-engine" CACHE PATH "Local path to helios-engine repository")

if(NOT TARGET helios_math AND NOT TARGET helios::math)
    if(EXISTS "${HELIOS_MATH_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_MATH_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-math-build")
    else()
        FetchContent_Declare(helios_math
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-math.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_math)
    endif()
endif()

if(NOT TARGET helios)
    if(EXISTS "${HELIOS_ENGINE_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_ENGINE_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-engine-build")
    else()
        FetchContent_Declare(helios_engine
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-engine.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_engine)
    endif()
endif()

if(CMAKE_IFC_OUTPUT_DIRECTORY)
    file(MAKE_DIRECTORY "${CMAKE_IFC_OUTPUT_DIRECTORY}")
endif()


file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB_RECURSE MODULE_INTERFACE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx"
)

list(FILTER CPP_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")
list(FILTER MODULE_INTERFACE_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")


add_library(helios_opengl STATIC)
add_library(helios::opengl ALIAS helios_opengl)

# Keep the dependency explicit while engine still provides the aggregated helios target.
if(TARGET helios_math)
    add_dependencies(helios_opengl helios_math)
endif()


target_link_libraries(
        helios_opengl
        PUBLIC
        helios
        thirdparty::freetype
        thirdparty::opengl
        thirdparty::imgui
)

set_property(TARGET helios_opengl PROPERTY CXX_SCAN_FOR_MODULES ON)
target_compile_features(helios_opengl PUBLIC cxx_std_23)

target_sources(helios_opengl PRIVATE ${CPP_FILES})

if(MODULE_INTERFACE_FILES)
    target_sources(helios_opengl PUBLIC
            FILE_SET modules TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
            FILES ${MODULE_INTERFACE_FILES}
    )
endif()

target_include_directories(helios_opengl PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)