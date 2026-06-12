add_subdirectory(vendors)

add_library(ncore STATIC
	"Engine.h" "Engine.cpp"
	"Definitions.h"
    "modules/Config.h"
    "modules/MainLoop.h" "modules/MainLoop.cpp"
    "modules/Services.h"
    "modules/World.h" "modules/World.cpp"
    "modules/audio/AudioManager.h" "modules/audio/AudioManager.cpp"
    "modules/ecs/Component.h"
    "modules/ecs/Entity.h" "modules/ecs/Entity.cpp"
    "modules/ecs/System.h"
    "modules/ecs/components/RigidBodyComponent.h"
    "modules/ecs/components/SpriteComponent.h"
    "modules/ecs/components/TransformComponent.h"
    "modules/ecs/systems/AudioSystem.h" "modules/ecs/systems/AudioSystem.cpp"
    "modules/ecs/systems/InputSystem.h" "modules/ecs/systems/InputSystem.cpp"
    "modules/ecs/systems/PhysicsSystem.h" "modules/ecs/systems/PhysicsSystem.cpp"
    "modules/ecs/systems/RenderSystem.h" "modules/ecs/systems/RenderSystem.cpp"
    "modules/events/Event.h"
    "modules/events/EventBus.h"
    "modules/events/EventType.h"
    "modules/events/InputEvent.h"
    "modules/graphics/Camera.h" "modules/graphics/Camera.cpp"
    "modules/graphics/ICamera.h"
    "modules/graphics/IGraphicsContext.h" "modules/graphics/IGraphicsContext.cpp"
    "modules/graphics/Viewport.h" "modules/graphics/Viewport.cpp"
    "modules/graphics/Window.h" "modules/graphics/Window.cpp"
    "modules/gui/Gui.h" "modules/gui/Gui.cpp"
    "modules/physics/Physics.h" "modules/physics/Physics.cpp"
	"modules/resources/Resource.h"
	"modules/resources/ResourceHandle.h"
    "modules/resources/ResourceManager.h" "modules/resources/ResourceManager.cpp"
    "modules/resources/ResourcePool.h"
	"modules/resources/IResourceLoader.h"
    "platform/sdl3/graphics/SDLGraphicsContext.h" "platform/sdl3/graphics/SDLGraphicsContext.cpp"
    "platform/sdl3/events/EventBackendSDL.h" "platform/sdl3/events/EventBackendSDL.cpp"
	"platform/sdl3/resources/SDLImageLoader.h" "platform/sdl3/resources/SDLImageLoader.cpp"
    "platform/sdl3/resources/SDLAudioLoader.h" "platform/sdl3/resources/SDLAudioLoader.cpp"
    "platform/imgui/ImGuiSDLRenderer.h" "platform/imgui/ImGuiSDLRenderer.cpp"
    "platform/imgui/ImGuiSDL.h" "platform/imgui/ImGuiSDL.cpp"
	"utils/logger/LogLevel.h"
	"utils/logger/LogChannel.h" "utils/logger/LogChannel.cpp"
    "utils/logger/Logger.h" "utils/logger/Logger.cpp"
	"utils/logger/Sink.h" "utils/logger/Sink.cpp"
	"utils/logger/AssertHandler.h" "utils/logger/AssertHandler.cpp"
	"utils/Macro.h"
	"utils/Assert.h"
	"utils/Color.h"
    "utils/ErrorCodes.h"
    "utils/Structures.h" "utils/Structures.cpp"
    "utils/Random.h"
)

target_precompile_headers(ncore PRIVATE "Definitions.h")

target_include_directories(ncore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(ncore PRIVATE "_CRT_SECURE_NO_WARNINGS")

target_link_libraries(ncore PUBLIC
    "ImGui"
    "box2d::box2d"
    "spdlog::spdlog"
    "SDL3::SDL3"
    "inifile-cpp"
    "$<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>"
)

set_target_properties(ncore PROPERTIES
	DEBUG_POSTFIX "_d"
	VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:ncore>"
)

target_compile_definitions(ncore PRIVATE
    "$<$<CONFIG:Debug>:DEBUG>"
    "$<$<CONFIG:Release>:RELEASE>"
    "$<$<CONFIG:Dist>:DIST>"
)
