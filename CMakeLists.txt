cmake_minimum_required(VERSION 4.0)
project(helios-opengl LANGUAGES CXX)

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
        $<BUILD_INTERFACE:${HELIOS_OPENGL_GENERATED_INCLUDE_DIR}>
)