#include "ecs_debug.h"

#include <imgui.h>

#include <ncore/game_world.h>
#include <ncore/modules/module_registry.h>
#include <ncore/modules/video/render_module.h>
#include <ncore/modules/video/window_module.h>
#include <ncore/runtime/components/ecs_time.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

static void
update_window_title( WindowModule* window, EcsEntityId eid, EcsWindow* window_instance, double fps, double delta_time )
{
    const std::string full_title = std::format(
        "{} (DEBUG) - EID {} - WID {} - FPS: {:.2f} - Delta: {:.6f}", window_instance->title, eid, window_instance->id,
        fps, delta_time
    );
    window->window_set_title( window_instance->id, full_title );
}

struct DebugState {
    std::array<char, 64> window_attrs;
    NSTRUCT( DebugState, NC_F( DebugState, window_attrs ) )
};

void EcsDebugFeature::build( EcsWorld& world )
{
    world.emplace_singleton<DebugState>();

    world.create_system( "EcsDebugFeature::DebugUI" )
        .with<EcsSwapChainRef>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto time = ctx.world().get_singleton<EcsTime>();
            auto gfx  = ctx.world().get_singleton<GraphicsModules>();

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu( "File" )) {
                    if (ImGui::MenuItem( "Quit", "Alt+F4" )) {
                        ctx.world().get_parent().request_quit();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            ImGui::Begin( "Debug" );

            ImGui::SeparatorText( "Time" );
            ImGui::Text( "Ticks: %u", time->ticks );
            ImGui::Text( "FPS: %f", time->fps );
            ImGui::Text( "Frame count: %d", time->frame_count );

            ImGui::SeparatorText( "RTTI" );
            ImGui::Text( "Hits: %d", rtti::TypeRegistry::get_rtti_hits() );

            ImGui::SeparatorText( "Rendering" );
            {
                const auto& stats = gfx->renderer->get_stats();
                ImGui::Text( "GPU Duration: %.3f ms", stats.gpu_duration_ms );
                ImGui::Text( "Input Assembler Primitives: %llu", stats.input_primitives );
                ImGui::Text( "Input Assembler Vertices: %llu", stats.input_vertices );
                ImGui::Text( "Vertex Shader Invocations: %llu", stats.vs_invocations );
                ImGui::Text( "Pixel Shader Invocations: %llu", stats.ps_invocations );
                ImGui::Text( "Clipping Primitives: %llu", stats.clipping_primitives );
                ImGui::Text( "Clipping Invocations: %llu", stats.clipping_invocations );
            }

            ImGui::SeparatorText( "ECS Debug" );
            ImGui::Text(
                "Entity count:\n Total: %zu\n Alive: %zu", ctx.world().get_entity_count(),
                ctx.world().get_entity_count( true )
            );

            if (ImGui::Button( "Spawn Window" )) {
                ctx.world()
                    .create_entity()
                    .with<EcsWindow>( EcsWindow{ .resolution = Vec2( 300, 300 ), .visible = true } )
                    .build();
            }

            ImGui::End();
        } );

#if !defined( NC_DIST )
    world.create_system( "EcsDebugFeature::HotReload" ).in( EcsSystemPhase::UPDATE ).run( []( QueryContext& ctx ) {
        if (ImGui::IsKeyPressed( ImGuiKey_F5 )) {
            NC_LOG_INFO_C( log::GRAPHICS, "Hot-reloading" );
        }
    } );
#endif

    // TODO: refactor this to use Timers
    world.create_system( "EcsDebugFeature::TitleBarUpdater" )
        .with<EcsWindow>()
        .in( EcsSystemPhase::POST_FRAME )
        .each( []( QueryContext& ctx, EcsEntityId id ) {
            auto gfx  = ctx.world().get_singleton<GraphicsModules>();
            auto time = ctx.world().get_singleton<EcsTime>();

            auto window = ctx.get_component<EcsWindow>();
            if (time->accumulator >= 0.5) {
                update_window_title( gfx->window, id, window, time->fps, ctx.delta_time() );
            }
        } );
}

} // namespace nc
