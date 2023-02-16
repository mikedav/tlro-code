cmake_minimum_required(VERSION 3.4)
project(idle)

set(CMAKE_BUILD_TYPE Debug)



set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})

find_package(MagnumIntegration REQUIRED ImGui)
find_package(Corrade REQUIRED Main PluginManager Utility)
find_package(Magnum REQUIRED GL Sdl2Application Trade SceneGraph Shaders MeshTools Primitives)
find_package(MagnumPlugins REQUIRED JpegImporter)
find_package(fmt REQUIRED)
find_package(GMP REQUIRED)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(Resources resources.conf)

add_executable(idle game.cpp nlohmann/json.hpp ${Resources})
target_link_libraries(idle PRIVATE
    Corrade::Main
    Corrade::PluginManager
    Corrade::Utility
    Magnum::GL
    Magnum::Magnum
    Magnum::Sdl2Application
    Magnum::Trade
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::MeshTools
    Magnum::Primitives
    MagnumIntegration::ImGui
    gmp
    fmt::fmt)

install(TARGETS idle DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT magnum-imgui)
