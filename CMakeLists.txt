cmake_minimum_required(VERSION 3.16)
project(eons LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define build configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Configs" FORCE)
set(CMAKE_C_FLAGS_DIST "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "C flags for Dist builds" FORCE)
set(CMAKE_CXX_FLAGS_DIST "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "C++ flags for Dist builds" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "Executable linker flags for Dist builds" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DIST "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "Shared linker flags for Dist builds" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_DIST "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}" CACHE STRING "Static linker flags for Dist builds" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_DIST "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}" CACHE STRING "Module linker flags for Dist builds" FORCE)

if(MSVC)
    add_compile_options(/FS /wd5105)
endif()

# Output directories
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
foreach(CONFIG_TYPE Debug Release Dist)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${OUTPUT_DIR}/${CONFIG_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${OUTPUT_DIR}/${CONFIG_TYPE})
endforeach()

add_subdirectory(vendors)

# Create executable with source files
add_executable(eons "src/main.cpp"
    "src/config.h"
    "src/engine/audio/AudioManager.h" "src/engine/audio/AudioManager.cpp"
    "src/engine/core/EntitySystem.h" "src/engine/core/EntitySystem.cpp"
    "src/engine/core/Logger.h" "src/engine/core/Logger.cpp"
    "src/engine/core/MainLoop.h" "src/engine/core/MainLoop.cpp"
    "src/engine/core/Random.h"
    "src/engine/core/Vector2D.h" "src/engine/core/Vector2D.cpp"
    "src/engine/core/Window.h" "src/engine/core/Window.cpp"
	"src/engine/core/Event.h" "src/engine/core/Event.cpp"
    "src/engine/graphics/PrimitiveShape.h"
    "src/engine/graphics/TextureManager.h" "src/engine/graphics/TextureManager.cpp"
    "src/engine/graphics/Viewport.h" "src/engine/graphics/Viewport.cpp"
    "src/engine/graphics/ICamera.h"
    "src/engine/graphics/Camera.h" "src/engine/graphics/Camera.cpp"
    "src/engine/graphics/IGraphicsContext.h"
    "src/engine/graphics/backends/SDLGraphicsContext.h"
    "src/engine/gui/backends/ImGuiSDL.h" "src/engine/gui/backends/ImGuiSDL.cpp"
    "src/engine/gui/backends/ImGuiSDLRenderer.h" "src/engine/gui/backends/ImGuiSDLRenderer.cpp"
    "src/engine/gui/ImGuiLayer.h" "src/engine/gui/ImGuiLayer.cpp"
    "src/engine/physics/Physics.h" "src/engine/physics/Physics.cpp"
    "src/engine/scene/Scene.h" "src/engine/scene/Scene.cpp"
    "src/engine/scene/TransformComponent.h" "src/engine/scene/TransformComponent.cpp"
    "src/engine/scene/RigidBodyComponent.h" "src/engine/scene/RigidBodyComponent.cpp"
    "src/engine/scene/SpriteComponent.h" "src/engine/scene/SpriteComponent.cpp"
    "src/game/experimental/NeuralNetwork/neural_network.h" "src/game/experimental/NeuralNetwork/neural_network.cpp"
    "src/game/microcosm/Evolution.cpp"
    "src/game/microcosm/evolution.h"
    "src/game/microcosm/Genes.h"
    "src/game/microcosm/Gui.h" "src/game/microcosm/Gui.cpp"
    "src/game/microcosm/Microcosm.h" "src/game/microcosm/Microcosm.cpp"
    "src/game/microcosm/Nutrient.h" "src/game/microcosm/Nutrient.cpp"
    "src/game/microcosm/Organism.h" "src/game/microcosm/Organism.cpp"
    "src/game/microcosm/OrganismAI.h" "src/game/microcosm/OrganismAI.cpp"
    "src/game/microcosm/Species.h" "src/game/microcosm/Species.cpp"
)

# Define preprocessor macros
target_compile_definitions(eons PRIVATE _CRT_SECURE_NO_WARNINGS)

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image CONFIG REQUIRED)

# Link external libraries
target_link_libraries(eons PRIVATE
        ImGui
        box2d::box2d
        spdlog::spdlog
        SDL3::SDL3
        $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
)

# Platform-specific settings
if(WIN32)
    set(CMAKE_SYSTEM_VERSION latest)
endif()

if(MINGW)
    message(STATUS "Detected MinGW, enabling static linking for libgcc and libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Build configurations
set_property(TARGET eons PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:eons>")

# Add Debug Configuration
set_target_properties(eons PROPERTIES DEBUG_POSTFIX "_d")
target_compile_definitions(eons PRIVATE $<$<CONFIG:Debug>:DEBUG>)

# Add Release Configuration
target_compile_definitions(eons PRIVATE $<$<CONFIG:Release>:RELEASE>)
target_compile_options(eons PRIVATE
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2>
        $<$<AND:$<CONFIG:Release>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-O3>
)

# Add Distribution Configuration
target_compile_definitions(eons PRIVATE $<$<CONFIG:Dist>:DIST>)
target_compile_options(eons PRIVATE
        $<$<AND:$<CONFIG:Dist>,$<CXX_COMPILER_ID:MSVC>>:/O2>
        $<$<AND:$<CONFIG:Dist>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-O3>
)

target_include_directories(eons PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/engine/audio
        ${CMAKE_SOURCE_DIR}/src/engine/core
        ${CMAKE_SOURCE_DIR}/src/engine/graphics
        ${CMAKE_SOURCE_DIR}/src/engine/graphics/backends
        ${CMAKE_SOURCE_DIR}/src/engine/gui
        ${CMAKE_SOURCE_DIR}/src/engine/gui/backends
        ${CMAKE_SOURCE_DIR}/src/engine/physics
        ${CMAKE_SOURCE_DIR}/src/engine/scene
        ${CMAKE_SOURCE_DIR}/src/game
        ${CMAKE_SOURCE_DIR}/src/game/experimental
        ${CMAKE_SOURCE_DIR}/src/game/microcosm
        ${CMAKE_SOURCE_DIR}/vendors/imgui
)

add_custom_command(TARGET eons POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:eons>/assets
)
