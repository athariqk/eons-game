// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// TODO: we need to decouple this class from SDL

#include <chrono>
#include <memory>
#include <sstream>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>

#include <ncore/application.h>
#include <ncore/core/memory.h>
#include <ncore/core/types.h>
#include <ncore/game_world.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/audio/audio_service.h>
#include <ncore/services/events/event_bus.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/window_service.h>
#include <ncore/utils/config.h>
#include <ncore/utils/log.h>
#include <utils/logger/logger.h>
#include <utils/logger/sink.h>
#include <utils/profiler.h>

namespace nc {

Application::Application( const AppDesc& p_desc )
{
    context.app_desc = p_desc;
}

Application::~Application()
{
    NC_ASSERT_MSG( !context.is_running, "Application destroyed while still running" );
}

void Application::init()
{
    NC_ZONE_COLOR( "Application::init", NC_COLOR_APP )

    rtti::TypeRegistry::initialize();

    context.config.load( context.app_desc.config_file );
    auto log_cfg = context.config.read<cfg::Log>();

    // Set up logging
    log::Logger::get_instance().add_sink( Ref<log::FileSink>::create( log_cfg.file_path ) );
    log::Logger::get_instance().set_level( log::Level( log_cfg.level ) );
    log::Level min_lvl = log::Level( log_cfg.level );
    if (!log_cfg.overrides.empty()) {
        std::istringstream stream( log_cfg.overrides );
        std::string pair;
        while (std::getline( stream, pair, ',' )) {
            auto sep = pair.find( ':' );
            if (sep != std::string::npos) {
                auto cat = pair.substr( 0, sep );
                auto lvl = std::stoi( pair.substr( sep + 1 ) );
                log::Logger::get_instance().set_level( cat, log::Level( lvl ) );
                if (lvl < static_cast<int>( min_lvl ))
                    min_lvl = log::Level( lvl );
            }
        }
    }
    log::set_min_level( min_lvl );

    NC_LOG_INFO(
        "Application starting: {} v{}.{}.{}-{}", context.app_desc.name, context.app_desc.version.major,
        context.app_desc.version.minor, context.app_desc.version.patch, context.app_desc.version.identifier
    );

    register_services();
    context.services.init_all( context.config );

    game_world = create_world();

    game_world->app_ctx = &context;
    game_world->on_enter();

    NC_LOG_TRACE( "Application initialized" );
}

static void throttle_framerate( std::chrono::steady_clock::time_point& p_cur_time, double p_target_frame_time )
{
    NC_ZONE_NAMED( "Application::throttle_framerate" )

    auto frame_end_time      = std::chrono::steady_clock::now();
    double actual_frame_time = std::chrono::duration<double>( frame_end_time - p_cur_time ).count();
    if (actual_frame_time < p_target_frame_time) {
        double sleep_duration = p_target_frame_time - actual_frame_time;
        std::this_thread::sleep_for( std::chrono::duration<double>( sleep_duration ) );
    }
}

void Application::run()
{
    constexpr double FIXED_DT        = 1.0 / 60.0;
    constexpr double MAX_ACCUMULATOR = FIXED_DT * 5.0;

    constexpr double TARGET_FPS        = 120.0;
    constexpr double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

    auto last_time     = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    context.is_running = true;
    while (context.is_running) {
        auto cur_time      = std::chrono::high_resolution_clock::now();
        context.delta_time = std::chrono::duration<double>( cur_time - last_time ).count();
        last_time          = cur_time;

        if (context.delta_time > MAX_ACCUMULATOR)
            context.delta_time = MAX_ACCUMULATOR;

        accumulator += context.delta_time;

        process_events();

        while (accumulator >= FIXED_DT) {
            NC_ZONE_NAMED( "Application::run::fixed_update" )
            if (game_world->on_fixed_update( FIXED_DT )) {
                break;
            }
            accumulator -= FIXED_DT;
            context.ticks++;
        }

        {
            NC_ZONE_NAMED( "Application::run::variable_update" )
            if (game_world->on_variable_update( context.delta_time )) {
                break;
            }
        }

        throttle_framerate( cur_time, TARGET_FRAME_TIME );

        NC_FRAME_MARK;
    }

    context.is_running = false;
}

void Application::process_events()
{
    NC_ZONE_NAMED( "Application::process_events" )

    SDL_PumpEvents();

    SDL_Event quit_event;
    while (SDL_PeepEvents( &quit_event, 1, SDL_GETEVENT, SDL_EVENT_QUIT, SDL_EVENT_QUIT ) > 0) {
        game_world->request_quit();
    }
}

void Application::register_services()
{
    NC_ZONE_NAMED( "Application::register_services" )

    SDL_SetMemoryFunctions(
        []( size_t size ) -> void* { return memalloc( size ); },
        []( size_t nmemb, size_t size ) -> void* { return memcalloc( nmemb, size ); },
        []( void* ptr, size_t size ) -> void* { return memrealloc( ptr, size ); }, []( void* ptr ) { memfree( ptr ); }
    );

    if (!SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO )) {
        NC_LOG_ERROR( "SDL init FAIL: {}", SDL_GetError() );
        abort(); // TODO: handle this more gracefully
    }

    events   = context.services.provide<EventBus>();
    input    = context.services.provide<InputService>();
    window   = context.services.provide<WindowService>();
    renderer = context.services.provide<RenderService>();
    context.services.provide<AudioService>();
    context.services.provide<AssetService>();
}

void Application::unregister_services()
{
    context.services.clear();
    SDL_Quit();
}

Ptr<IGameWorld> Application::create_world()
{
    return std::make_unique<Scene>();
}

void Application::finish()
{
    NC_ZONE_COLOR( "Application::finish", NC_COLOR_APP )

    NC_LOG_TRACE( "Application teardown" );
    game_world->on_exit();
    game_world.reset(); // destroy heap allocations.
    context.services.cleanup_all();
    unregister_services();
    rtti::TypeRegistry::shutdown();
}

} // namespace nc
