#include "ecs_debug.h"

#include <imgui.h>

#include <ncore/modules/module_registry.h>
#include <ncore/modules/video/video_module.h>
#include <ncore/runtime/components/ecs_time.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

static void update_window_title( VideoModule* video, EcsEntityId eid, EcsWindow* window, double fps, double delta_time )
{
    const std::string full_title = std::format(
        "{} (DEBUG) - EID {} - WID {} - FPS: {:.2f} - Delta: {:.6f}", window->title, eid, window->id, fps, delta_time
    );
    video->set_window_title( window->id, full_title );
}

struct DebugState {
    std::array<char, 64> window_attrs;
    VideoModule* video = nullptr;

    NSTRUCT( DebugState, NC_F( DebugState, window_attrs ) NC_F( DebugState, video ) )
};

void EcsDebugFeature::build( EcsWorld& world )
{
    world.emplace_singleton<DebugState>();

    world.create_system( "EcsDebugFeature::Init" )
        .with<DebugState>()
        .in( EcsSystemPhase::Init )
        .run( []( QueryContext& ctx ) {
            auto state   = ctx.get_component<DebugState>();
            state->video = ctx.modules().resolve<VideoModule>();
        } );

    world.create_system( "EcsDebugFeature::StatsUpdater" )
        .with<EcsRenderSurface>()
        .in( EcsSystemPhase::Update )
        .run( []( QueryContext& ctx ) {
            auto& time = ctx.world().get_singleton<EcsTime>();

            ImGui::Begin( "Debug" );

            ImGui::SeparatorText( "Time" );
            ImGui::Text( "Ticks: %u", time.ticks );
            ImGui::Text( "FPS: %f", time.fps );
            ImGui::Text( "Frame count: %d", time.frame_count );

            ImGui::SeparatorText( "RTTI" );
            ImGui::Text( "Hits: %d", rtti::TypeRegistry::get_rtti_hits() );

            ImGui::SeparatorText( "Rendering" );
            ImGui::Text( "Stub" );

            ImGui::SeparatorText( "ECS Debug" );
            ImGui::Text(
                "Entity count:\n Total: %zu\n Alive: %zu", ctx.world().get_entity_count(),
                ctx.world().get_entity_count( true )
            );

            if (ImGui::Button( "Spawn Window", ImVec2( 100.0f, 20.0f ) )) {
                ctx.world()
                    .create_entity()
                    .with<EcsWindow>( EcsWindow{ .resolution = Vec2( 300, 300 ), .visible = true } )
                    .build();
            }

            ImGui::End();
        } );

    // TODO: refactor this to use Timers
    world.create_system( "EcsDebugFeature::TitleBarUpdater" )
        .with<EcsWindow>()
        .in( EcsSystemPhase::PostFrame )
        .each( []( QueryContext& ctx, EcsEntityId id ) {
            auto& state = ctx.world().get_singleton<DebugState>();
            auto& time  = ctx.world().get_singleton<EcsTime>();

            auto window = ctx.get_component<EcsWindow>();

            if (time.accumulator >= 0.5) {
                update_window_title( state.video, id, window, time.fps, ctx.delta_time() );
            }
        } );
}

} // namespace nc
