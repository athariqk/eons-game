// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <chrono>
#include <format>
#include <memory>
#include <sstream>

#include <SDL3/SDL_init.h>
#include <backends/box2d/box2d_physics_impl.h>
#include <backends/dear-imgui/dear_imgui_impl.h>
#include <backends/sdl/sdl_audio_loader.h>
#include <backends/sdl/sdl_image_loader.h>
#include <backends/sdl/sdl_type_helpers.h>
#include <backends/sdl/sdl_window_impl.h>
#include <backends/vulkan/vk_render_backend.h>

#include <ncore/application.h>
#include <ncore/game_world.h>
#include <ncore/kernel/types.h>
#include <ncore/modules/audio/audio_module.h>
#include <ncore/modules/events/event_bus.h>
#include <ncore/modules/module_registry.h>
#include <ncore/modules/resource/resource_manager.h>
#include <ncore/modules/video/graphics_module.h>
#include <ncore/modules/video/viewport.h>
#include <ncore/runtime/ecs_runtime.h>
#include <ncore/scene/scene.h>
#include <ncore/utils/config.h>
#include <ncore/utils/log.h>
#include <utils/logger/log_level.h>
#include <utils/logger/logger.h>
#include <utils/logger/sink.h>

namespace nc {

namespace cfg {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

struct Log {
    int Level            = 0;
    std::string FilePath = "logs/engine.log";
    std::string Overrides;
    NSTRUCT( Log, NC_F( Log, Level ) NC_F( Log, FilePath ) NC_F( Log, Overrides ) )
};

#pragma GCC diagnostic pop

struct Window {
    int SizeWidth   = 800;
    int SizeHeight  = 800;
    bool Fullscreen = false;
    NSTRUCT( Window, NC_F( Window, SizeWidth ) NC_F( Window, SizeHeight ) NC_F( Window, Fullscreen ) )
};

struct Render {
    bool VSync           = true;
    float PixelsPerMeter = 32.0f;
    NSTRUCT( Render, NC_F( Render, VSync ) NC_F( Render, PixelsPerMeter ) )
};

} // namespace cfg

Application::Application( const AppDesc& desc ) : app_desc( desc ) {}

Application::~Application()
{
    NC_ASSERT( !is_running, "Application destroyed while still running" );
}

void Application::init()
{
    rfl::Registry::register_primitive_types();

    auto cfg_file = ConfFile( app_desc.ConfigFile );
    auto log_cfg  = cfg_file.read<cfg::Log>();

    // Set up logging
    log::Logger::get_instance().add_sink( std::make_shared<log::FileSink>( log_cfg.FilePath ) );
    log::Logger::get_instance().set_level( log::Level( log_cfg.Level ) );
    std::string_view overrides( log_cfg.Overrides );
    if (!overrides.empty()) {
        std::istringstream stream( log_cfg.Overrides );
        std::string pair;
        while (std::getline( stream, pair, ',' )) {
            auto sep = pair.find( ':' );
            if (sep != std::string::npos) {
                auto cat = pair.substr( 0, sep );
                auto lvl = std::stoi( pair.substr( sep + 1 ) );
                log::Logger::get_instance().set_level( cat, log::Level( lvl ) );
            }
        }
    }

    register_modules( cfg_file );
    modules.init_all();

    g_world = create_world();
    g_world->on_init();
    on_world_init( *g_world );

    NC_LOG_TRACE( "Application initialized" );
}

static void update_window_title(
    IWindowModule* window, RID main_win, const std::string& base_title, char* window_attrs, double fps,
    double delta_time
)
{
    std::snprintf( window_attrs, 64, "FPS: %.2f - Delta: %.6f", fps, delta_time );
    const std::string full_title = std::format( "{} - {}", base_title, window_attrs );
    window->set_title( main_win, full_title );
}

static void throttle_framerate( std::chrono::steady_clock::time_point& cur_time, double target_frame_time )
{
    auto frame_end_time      = std::chrono::steady_clock::now();
    double actual_frame_time = std::chrono::duration<double>( frame_end_time - cur_time ).count();
    if (actual_frame_time < target_frame_time) {
        double sleep_duration = target_frame_time - actual_frame_time;
        std::this_thread::sleep_for( std::chrono::duration<double>( sleep_duration ) );
    }
}

void Application::run()
{
    constexpr double FIXED_DT        = 1.0 / 60.0;
    constexpr double MAX_ACCUMULATOR = FIXED_DT * 5.0;

    constexpr double TARGET_FPS        = 120.0;
    constexpr double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

    auto last_time            = std::chrono::high_resolution_clock::now();
    auto last_fps_update_time = std::chrono::high_resolution_clock::now();

    int frame_count    = 0;
    double accumulator = 0.0;

    std::array<char, 64> window_attrs;

    auto main_win = windows->get_main_window_id();
    NC_ASSERT_RET( main_win.is_valid(), "Main window is not valid" );

    is_running = true;
    while (is_running) {
        auto cur_time = std::chrono::high_resolution_clock::now();
        delta_time    = std::chrono::duration<double>( cur_time - last_time ).count();
        last_time     = cur_time;

        if (delta_time > MAX_ACCUMULATOR)
            delta_time = MAX_ACCUMULATOR;

        accumulator += delta_time;

        events->process_queue();
        poll_events();

        while (accumulator >= FIXED_DT) {
            if (g_world->on_fixed_update( FIXED_DT )) {
                break;
            }
            accumulator -= FIXED_DT;
            ticks++;
        }

        if (g_world->on_variable_update( delta_time )) {
            break;
        }

        double elapsed = std::chrono::duration<double>( cur_time - last_fps_update_time ).count();
        if (elapsed >= 1.0) {
            double fps           = frame_count / elapsed;
            frame_count          = 0;
            last_fps_update_time = cur_time;
            update_window_title( windows, main_win, app_desc.Name, window_attrs.data(), fps, delta_time );
        }

        throttle_framerate( cur_time, TARGET_FRAME_TIME );

        frame_count++;
    }

    is_running = false;
}

void Application::poll_events()
{
    SDL_Event sdl_event;
    while (SDL_PollEvent( &sdl_event )) {
        auto event = SDLTypeHelpers::map_from_sdl( sdl_event );
        imgui->process_event( event.get() );
        // TODO: make this more proper
        if (gfx && event->get_type() == EventType::WINDOW_RESIZE) {
            auto e = static_cast<WindowResizeEvent*>( event.get() );
            gfx->set_render_size( Vec2( static_cast<float>( e->width ), static_cast<float>( e->height ) ) );
        }
        events->enqueue( std::move( event ) );
    }
}

void Application::register_modules( ConfFile& cfg_file )
{
    auto window_cfg = cfg_file.read<cfg::Window>();
    auto render_cfg = cfg_file.read<cfg::Render>();

    if (!SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO )) {
        NC_LOG_ERROR( "SDL init FAIL: {}", SDL_GetError() );
        abort(); // TODO: handle this more gracefully
    }

    // Event bus
    events = modules.provide<EventBus>();

    // Set up resource management
    auto resources = modules.provide<ResourceManager>();
    resources->register_importer<SDLAudioLoader>();
    resources->register_importer<SDLImageLoader>();

    windows = modules.provide<SDLWindowImpl>();
    // main window is created here
    windows->create_window(
        app_desc.Name, Vec2( static_cast<float>( window_cfg.SizeWidth ), static_cast<float>( window_cfg.SizeHeight ) ),
        window_cfg.Fullscreen
    );
    // set the pixels per meter for the main window's viewport
    windows->get_viewport()->set_pixels_per_meter(
        render_cfg.PixelsPerMeter
    ); // TODO: properly implement viewport later

    auto renderer = std::make_unique<VkRenderBackend>( windows );
    gfx           = modules.provide<GraphicsModule>( std::move( renderer ) );
    gfx->set_vsync( render_cfg.VSync );

    modules.provide<Box2DPhysicsImpl>();
    modules.provide<AudioModule>();

    imgui = modules.provide<DearImGuiImpl>( gfx, windows );
}

void Application::unregister_modules()
{
    modules.clear();
    SDL_Quit();
}

std::unique_ptr<IGameWorld> Application::create_world()
{
    auto scene = std::make_unique<Scene>( modules );
    scene->get_ecs().load_feature<EcsRuntimeFeature>();
    return scene;
}

void Application::finish()
{
    g_world->on_finish();
    modules.cleanup_all();
    unregister_modules();
    NC_LOG_TRACE( "Application finished" );
}

} // namespace nc
