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
    add_compile_options(/FS /wd5105 /Zc:preprocessor)
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
	"src/engine/Engine.h" "src/engine/Engine.cpp"
    "src/engine/audio/AudioManager.h" "src/engine/audio/AudioManager.cpp"
    "src/engine/core/World.h" "src/engine/core/World.cpp"
    "src/engine/core/Logger.h" "src/engine/core/Logger.cpp"
    "src/engine/core/MainLoop.h" "src/engine/core/MainLoop.cpp"
    "src/engine/core/Random.h"
    "src/engine/core/Services.h"
    "src/engine/core/Vector2D.h" "src/engine/core/Vector2D.cpp"
    "src/engine/core/Window.h" "src/engine/core/Window.cpp"
	"src/engine/core/ErrorCodes.h"
	"src/engine/events/InputEvents.h" "src/engine/events/InputEvents.cpp"
    "src/engine/events/EventBus.h"
	"src/engine/ecs/Entity.h" "src/engine/ecs/Entity.cpp"
	"src/engine/ecs/Component.h"
	"src/engine/ecs/System.h"
	"src/engine/components/RigidBodyComponent.h"
	"src/engine/components/TransformComponent.h"
	"src/engine/components/SpriteComponent.h"
	"src/engine/systems/PhysicsSystem.h" "src/engine/systems/PhysicsSystem.cpp"
	"src/engine/systems/RenderSystem.h" "src/engine/systems/RenderSystem.cpp"
	"src/engine/systems/AudioSystem.h" "src/engine/systems/AudioSystem.cpp"
	"src/engine/systems/InputSystem.h" "src/engine/systems/InputSystem.cpp"
	"src/engine/graphics/PrimitiveShape.h"
    "src/engine/graphics/TextureManager.h" "src/engine/graphics/TextureManager.cpp"
    "src/engine/graphics/Viewport.h" "src/engine/graphics/Viewport.cpp"
    "src/engine/graphics/ICamera.h"
    "src/engine/graphics/Camera.h" "src/engine/graphics/Camera.cpp"
    "src/engine/graphics/IGraphicsContext.h"
    "src/engine/graphics/backends/SDLGraphicsContext.h"
    "src/engine/gui/backends/ImGuiSDL.h" "src/engine/gui/backends/ImGuiSDL.cpp"
    "src/engine/gui/backends/ImGuiSDLRenderer.h" "src/engine/gui/backends/ImGuiSDLRenderer.cpp"
    "src/engine/gui/Gui.h" "src/engine/gui/Gui.cpp"
    "src/engine/physics/Physics.h" "src/engine/physics/Physics.cpp"
	"src/engine/utils/Config.h"
    "src/game/experimental/NeuralNetwork/neural_network.h" "src/game/experimental/NeuralNetwork/neural_network.cpp"
    "src/game/microcosm/Evolution.cpp"
    "src/game/microcosm/evolution.h"
    "src/game/microcosm/Genes.h"
    "src/game/microcosm/MicrocosmWorld.h" "src/game/microcosm/MicrocosmWorld.cpp"
    "src/game/microcosm/components/NutrientComponent.h"
    "src/game/microcosm/components/OrganismComponent.h" "src/game/microcosm/components/OrganismComponent.cpp"
    "src/game/microcosm/components/OrganismAIComponent.h" "src/game/microcosm/components/OrganismAIComponent.cpp"
    "src/game/microcosm/components/SpeciesComponent.h" "src/game/microcosm/components/SpeciesComponent.cpp"
	"src/game/microcosm/systems/NutrientSystem.h" "src/game/microcosm/systems/NutrientSystem.cpp"
    "src/game/microcosm/systems/OrganismSystem.h" "src/game/microcosm/systems/OrganismSystem.cpp"
    "src/game/microcosm/systems/OrganismAISystem.h" "src/game/microcosm/systems/OrganismAISystem.cpp"
    "src/game/microcosm/systems/SpeciesSystem.h" "src/game/microcosm/systems/SpeciesSystem.cpp"
)

# Define preprocessor macros
target_compile_definitions(eons PRIVATE _CRT_SECURE_NO_WARNINGS)

target_precompile_headers(eons PRIVATE "src/pch.h")

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image CONFIG REQUIRED)

# Link external libraries
target_link_libraries(eons PRIVATE
        ImGui
        box2d::box2d
        spdlog::spdlog
        SDL3::SDL3
		inifile-cpp
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
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src/engine/audio
        ${CMAKE_SOURCE_DIR}/src/engine/core
        ${CMAKE_SOURCE_DIR}/src/engine/ecs
        ${CMAKE_SOURCE_DIR}/src/engine/components
        ${CMAKE_SOURCE_DIR}/src/engine/systems
        ${CMAKE_SOURCE_DIR}/src/engine/events
        ${CMAKE_SOURCE_DIR}/src/engine/graphics
        ${CMAKE_SOURCE_DIR}/src/engine/graphics/backends
        ${CMAKE_SOURCE_DIR}/src/engine/gui
        ${CMAKE_SOURCE_DIR}/src/engine/gui/backends
        ${CMAKE_SOURCE_DIR}/src/engine/physics
        ${CMAKE_SOURCE_DIR}/src/engine/utils
        ${CMAKE_SOURCE_DIR}/src/game
        ${CMAKE_SOURCE_DIR}/src/game/experimental
        ${CMAKE_SOURCE_DIR}/src/game/microcosm
        ${CMAKE_SOURCE_DIR}/vendors/imgui
)

add_custom_command(
    TARGET eons POST_BUILD
    COMMAND ${CMAKE_COMMAND} 
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DTARGET_DIR=$<TARGET_FILE_DIR:eons>
        -P "${CMAKE_SOURCE_DIR}/cmake/copy_files.cmake"
    COMMENT "Running deployment scripts for assets and config"
    VERBATIM
)
