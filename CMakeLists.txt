cmake_minimum_required(VERSION 4.0)
project(helios-opengl LANGUAGES C CXX)

include(CTest)
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

if(NOT TARGET thirdparty::glad)
    FetchContent_Declare(glad
            GIT_REPOSITORY https://github.com/Dav1dde/glad.git
            GIT_TAG v2.0.8
            SOURCE_SUBDIR cmake
    )
    FetchContent_MakeAvailable(glad)
    glad_add_library(glad_gl_core_46 STATIC REPRODUCIBLE LOADER API gl:core=4.6)
    add_library(thirdparty::glad ALIAS glad_gl_core_46)
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
        $<BUILD_INTERFACE:helios>
        $<INSTALL_INTERFACE:helios::helios>
        PRIVATE
        $<BUILD_INTERFACE:thirdparty::freetype>
        $<BUILD_INTERFACE:thirdparty::glad>
        $<BUILD_INTERFACE:thirdparty::opengl>
        $<BUILD_INTERFACE:thirdparty::imgui>
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
        $<INSTALL_INTERFACE:include>
)

set(HELIOS_OPENGL_ENABLE_PACKAGE_DEFAULT OFF)
if(PROJECT_IS_TOP_LEVEL)
    set(HELIOS_OPENGL_ENABLE_PACKAGE_DEFAULT ON)
endif()
option(HELIOS_OPENGL_ENABLE_PACKAGE "Enable install/export package files" ${HELIOS_OPENGL_ENABLE_PACKAGE_DEFAULT})
if(HELIOS_OPENGL_ENABLE_PACKAGE)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS helios_opengl
            EXPORT helios-openglTargets
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            FILE_SET modules DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    set(HELIOS_OPENGL_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/helios-opengl")

    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helios-openglConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/helios-openglConfig.cmake"
            INSTALL_DESTINATION "${HELIOS_OPENGL_INSTALL_CMAKEDIR}"
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/helios-openglConfigVersion.cmake"
            VERSION "0.1.0"
            COMPATIBILITY SameMajorVersion
    )

    install(EXPORT helios-openglTargets
            FILE helios-openglTargets.cmake
            NAMESPACE helios::
            DESTINATION "${HELIOS_OPENGL_INSTALL_CMAKEDIR}"
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/helios-openglConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/helios-openglConfigVersion.cmake"
            DESTINATION "${HELIOS_OPENGL_INSTALL_CMAKEDIR}"
    )
endif()

if(BUILD_TESTING)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test.cpp")

    add_executable(helios-opengl-tests ${TEST_SOURCES})
    target_link_libraries(helios-opengl-tests PRIVATE helios_opengl GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(helios-opengl-tests)
endif()
