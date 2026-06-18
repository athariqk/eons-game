target_sources(ncore PRIVATE
	"kernel/object.cpp"
	"kernel/job_scheduler.h"
	"kernel/structures.cpp"
	"kernel/config.cpp"
	"kernel/types.cpp"
	"runtime/register_types.h" "runtime/register_types.cpp"
	"runtime/main_loop.cpp"
	"runtime/game_loop.cpp"
	"runtime/ecs/ecs_engine_module.cpp"
	"runtime/ecs/ecs_audio_system.cpp"
	"runtime/ecs/ecs_camera_system.cpp"
	"runtime/ecs/ecs_input_system.cpp"
	"runtime/ecs/ecs_physics_system.cpp"
	"runtime/ecs/ecs_render_system.cpp"
	"modules/assets/asset.cpp"
	"modules/assets/asset_pool.cpp"
	"modules/assets/asset_manager.cpp"
	"modules/assets/sdl_audio_loader.h" "modules/assets/sdl_audio_loader.cpp"
	"modules/assets/sdl_image_loader.h" "modules/assets/sdl_image_loader.cpp"
	"modules/audio/sdl_audio_impl.h" "modules/audio/sdl_audio_impl.cpp"
	"modules/ecs/ecs_world.cpp"
	"modules/events/event_bus.cpp"
	"modules/events/sdl_event_helpers.h"
	"modules/video/camera.cpp"
	"modules/video/render_service.cpp"
	"modules/video/sdl_render_impl.h" "modules/video/sdl_render_impl.cpp"
	"modules/video/sdl_window_impl.h" "modules/video/sdl_window_impl.cpp"
	"modules/video/viewport.cpp"
	"modules/gui/imgui_impl.h" "modules/gui/imgui_impl.cpp"
	"modules/gui/imgui_sdl.h" "modules/gui/imgui_sdl.cpp"
	"modules/gui/imgui_sdl_renderer.h" "modules/gui/imgui_sdl_renderer.cpp"
	"modules/physics/box2d_physics_impl.h" "modules/physics/box2d_physics_impl.cpp"
	"utils/logger/log_channel.cpp"
	"utils/logger/logger.cpp"
	"utils/logger/sink.cpp"
	"utils/logger/assert_handler.cpp"
)

target_include_directories(ncore
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/ncore
)
