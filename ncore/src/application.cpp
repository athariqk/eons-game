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
#include <backends/box2d/box2d_physics_impl.h>

#include <ncore/application.h>
#include <ncore/game_world.h>
#include <ncore/kernel/memory.h>
#include <ncore/kernel/types.h>
#include <ncore/modules/audio/audio_module.h>
#include <ncore/modules/events/event_bus.h>
#include <ncore/modules/input/input_module.h>
#include <ncore/modules/module_registry.h>
#include <ncore/modules/resource/resource_manager.h>
#include <ncore/modules/video/graphics_module.h>
#include <ncore/modules/video/video_module.h>
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

} // namespace cfg

Application::Application( const AppDesc& desc ) : app_desc( desc ) {}

Application::~Application()
{
    NC_ASSERT( !is_running, "Application destroyed while still running" );
}

void Application::init()
{
    rtti::TypeRegistry::initialize();

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

    register_modules();
    modules.init_all( cfg_file );

    g_world = create_world();
    g_world->on_init();
    on_world_init( *g_world );

    NC_LOG_TRACE( "Application initialized" );
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

    auto last_time     = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    is_running = true;
    while (is_running) {
        auto cur_time = std::chrono::high_resolution_clock::now();
        delta_time    = std::chrono::duration<double>( cur_time - last_time ).count();
        last_time     = cur_time;

        if (delta_time > MAX_ACCUMULATOR)
            delta_time = MAX_ACCUMULATOR;

        accumulator += delta_time;

        process_events();

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

        throttle_framerate( cur_time, TARGET_FRAME_TIME );
    }

    is_running = false;
}

void Application::process_events()
{
    SDL_PumpEvents();

    SDL_Event quit_event;
    while (SDL_PeepEvents( &quit_event, 1, SDL_GETEVENT, SDL_EVENT_QUIT, SDL_EVENT_QUIT ) > 0) {
        g_world->request_quit();
    }
}

void Application::register_modules()
{
    SDL_SetMemoryFunctions(
        []( size_t size ) -> void* { return memalloc( size ); },
        []( size_t nmemb, size_t size ) -> void* { return memcalloc( nmemb, size ); },
        []( void* ptr, size_t size ) -> void* { return memrealloc( ptr, size ); }, []( void* ptr ) { memfree( ptr ); }
    );

    if (!SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO )) {
        NC_LOG_ERROR( "SDL init FAIL: {}", SDL_GetError() );
        abort(); // TODO: handle this more gracefully
    }

    events    = modules.provide<EventBus>();
    input     = modules.provide<InputModule>();
    resources = modules.provide<ResourceManager>();
    video     = modules.provide<VideoModule>();
    gfx       = modules.provide<GraphicsModule>();
    modules.provide<Box2DPhysicsImpl>();
    modules.provide<AudioModule>();
}

void Application::unregister_modules()
{
    modules.clear();
    SDL_Quit();
}

std::unique_ptr<IGameWorld> Application::create_world()
{
    return std::make_unique<Scene>( app_desc, modules );
}

void Application::on_world_init( IGameWorld& world ) {}

void Application::finish()
{
    NC_LOG_TRACE( "Application teardown" );
    g_world->on_finish();
    g_world.reset();
    modules.cleanup_all();
    unregister_modules();
    rtti::TypeRegistry::shutdown();
}

} // namespace nc
