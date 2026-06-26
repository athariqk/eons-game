// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <chrono>
#include <format>
#include <memory>
#include <sstream>

#include <SDL3/SDL_init.h>

#include <kernel/config.h>
#include <modules/assets/asset_manager.h>
#include <modules/assets/sdl_audio_loader.h>
#include <modules/assets/sdl_image_loader.h>
#include <modules/audio/sdl_audio_impl.h>
#include <modules/events/sdl_event_helpers.h>
#include <modules/gui/gui_service.h>
#include <modules/gui/imgui_impl.h>
#include <modules/physics/box2d_physics_impl.h>
#include <modules/video/sdl_render_impl.h>
#include <modules/video/sdl_window_impl.h>
#include <ncore/application.h>
#include <ncore/kernel/types.h>
#include <ncore/modules/game_world.h>
#include <ncore/modules/service_locator.h>
#include <ncore/modules/video/image.h>
#include <ncore/modules/video/render_service.h>
#include <ncore/modules/video/viewport.h>
#include <ncore/runtime/ecs/ecs_runtime.h>
#include <ncore/scene/scene.h>
#include <ncore/utils/log.h>
#include <utils/logger/log_level.h>
#include <utils/logger/logger.h>
#include <utils/logger/sink.h>

namespace ncore {

namespace cfg {

struct Log {
    int Level            = 0;
    std::string FilePath = "logs/engine.log";
    std::string Overrides;
    NSTRUCT( Log, NC_F( Log, Level ) NC_F( Log, FilePath ) NC_F( Log, Overrides ) );
};

struct Window {
    int SizeWidth   = 800;
    int SizeHeight  = 800;
    bool Fullscreen = false;
    NSTRUCT( Window, NC_F( Window, SizeWidth ) NC_F( Window, SizeHeight ) NC_F( Window, Fullscreen ) );
};

struct Render {
    float PixelsPerMeter = 32.0f;
    NSTRUCT( Render, NC_F( Render, PixelsPerMeter ) );
};

} // namespace cfg

Application::Application( const AppDesc& desc ) : app_desc( desc ), services( ServiceLocator::get_instance() ) {}

Application::~Application()
{
    NC_ASSERT( !is_running, "application destroyed while still running" );
}

void Application::init()
{
    auto cfg_file   = ConfFile( app_desc.ConfigFile );
    auto log_cfg    = cfg_file.read<cfg::Log>();
    auto window_cfg = cfg_file.read<cfg::Window>();
    auto render_cfg = cfg_file.read<cfg::Render>();

    // Set up logging
    log::Logger::get_instance().add_sink( std::make_shared<log::FileSink>( log_cfg.FilePath ) );
    log::Logger::get_instance().set_level( log::Level( log_cfg.Level ) );
    if (!log_cfg.Overrides.empty()) {
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

    if (!SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO )) {
        NC_LOG_ERROR( "SDL init FAIL: {}", SDL_GetError() );
        abort(); // TODO: handle this more gracefully
    }

    event_bus = services.provide<EventBus>();

    auto resources = services.provide<AssetManager>();
    resources->register_loader<Image>( SDLImageLoader() );
    resources->register_loader<AudioClip>( SDLAudioLoader() );

    auto window = services.provide<SDLWindowImpl>(
        app_desc.Name.c_str(), window_cfg.SizeWidth, window_cfg.SizeHeight, window_cfg.Fullscreen
    );
    window->get_viewport()->set_pixels_per_meter(
        render_cfg.PixelsPerMeter
    ); // TODO: properly implement viewport later

    auto renderer = services.provide<SDLRenderImpl>( window->get_window_id() );
    auto physics  = services.provide<Box2DPhysicsImpl>();
    auto audio    = services.provide<SDLAudioImpl>();
    auto gui      = services.provide<ImGuiImpl>( window->get_window_id() );

    services.init_all();

    g_world = create_world();
    g_world->on_init();
    on_world_init( *g_world );

    NC_LOG_TRACE( "application initialized" );
}

void update_window_title(
    IWindowService* window, const std::string& base_title, char* window_attrs, double fps, double delta_time
)
{
    std::snprintf( window_attrs, 64, "FPS: %.2f - Delta: %.6f", fps, delta_time );
    const std::string full_title = std::format( "{} - {}", base_title, window_attrs );
    window->set_title( full_title.c_str() );
}

void throttle_framerate( std::chrono::steady_clock::time_point& cur_time, double target_frame_time )
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

    auto window   = services.resolve<SDLWindowImpl>();
    auto renderer = services.resolve<IRenderService>();
    auto gui      = services.resolve<IIMGuiService>();
    std::array<char, 64> window_attrs;

    is_running = true;
    while (is_running) {
        auto cur_time = std::chrono::high_resolution_clock::now();
        delta_time    = std::chrono::duration<double>( cur_time - last_time ).count();
        last_time     = cur_time;

        if (delta_time > MAX_ACCUMULATOR)
            delta_time = MAX_ACCUMULATOR;

        accumulator += delta_time;

        event_bus->process_queue();
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
            update_window_title( window, app_desc.Name, window_attrs.data(), fps, delta_time );
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
        auto event = SDLEventHelpers::map_from_sdl( sdl_event );
        event_bus->enqueue( std::move( event ) );
    }
}

std::unique_ptr<IGameWorld> Application::create_world()
{
    auto scene = std::make_unique<Scene>( services );
    scene->get_ecs().load_feature<EcsRuntimeFeature>();
    return scene;
}

void Application::finish()
{
    g_world->on_finish();
    services.cleanup_all();
    SDL_Quit();
    NC_LOG_TRACE( "application finished" );
}

} // namespace ncore
