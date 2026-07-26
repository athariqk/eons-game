#include "ecs_debug.h"

#include <imgui.h>

#include <ncore/game_world.h>
#include <ncore/modules/io/resource_manager.h>
#include <ncore/modules/video/render_module.h>
#include <ncore/modules/video/window_module.h>
#include <ncore/resources/material_template.h>
#include <ncore/runtime/components/ecs_material.h>
#include <ncore/runtime/components/ecs_sprite.h>
#include <ncore/runtime/components/ecs_time.h>
#include <ncore/runtime/components/ecs_transform.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_world.h>
#include <runtime/video/ecs_render_feature.h>

namespace nc {

// static void
// update_window_title( WindowModule* window, EcsEntityId eid, EcsWindow* window_instance, double fps, double delta_time
// )
//{
//     const std::string full_title = std::format(
//         "{} (DEBUG) - EID {} - WID {} - FPS: {:.2f} - Delta: {:.6f}", window_instance->title, eid,
//         window_instance->id, fps, delta_time
//     );
//     window->window_set_title( window_instance->id, full_title );
// }

struct DebugState {
    std::array<char, 64> window_attrs;
    NSTRUCT( DebugState, NC_F( DebugState, window_attrs ) )
};

void EcsDebugFeature::build( EcsWorld& world )
{
    world.emplace_singleton<DebugState>();

    world.create_observer( "EcsDebugFeature::CreateTestQuad" )
        .on<EcsSwapChainRef>( EcsCoreEvent::OnSet )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.world().get_singleton<IoModules>();
            auto sc = ctx.get_component<EcsSwapChainRef>();

            ctx.world()
                .create_entity( "TestQuad" )
                .with<EcsTransform2D>(
                    EcsTransform2D{ .position = Vec2( sc->size.x / 2, sc->size.y / 2 ), .size = Vec2( 150, 150 ) }
                )
                .with<EcsMaterialInstance>( EcsMaterialInstance{
                    .template_ref = io->resources->load<MaterialTemplate>( "engine/materials/canvas.material" )
                } )
                .with<EcsSpriteRenderer>( EcsSpriteRenderer{ .texture = 0, .tint = Color( 255, 125, 0, 255 ) } )
                .build();
        } );

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

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 work_pos               = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
            ImVec2 work_size              = viewport->WorkSize;

            {
                ImGui::SetNextWindowBgAlpha( 0.35f );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

                constexpr float PAD = 10.0f;

                ImGui::SetNextWindowPos(
                    ImVec2( ( work_pos.x + work_size.x ) - PAD, work_pos.y + PAD ), ImGuiCond_Always,
                    ImVec2( 1.0f, 0.0f )
                );

                ImGui::SetNextWindowSize( ImVec2( 200, 0 ) );

                bool open = true;
                if (ImGui::Begin( "Time", &open, window_flags )) {
                    ImGui::Text( "Ticks: %u", time->ticks );
                    ImGui::Text( "FPS: %.3f", time->fps );
                    ImGui::Text( "Frame count: %d", time->frame_count );
                }
                ImGui::End();

                ImGui::PopStyleVar();
            }

            {
                ImGui::SetNextWindowPos( ImVec2( work_pos.x, work_pos.y ), ImGuiCond_Always, ImVec2( 0.0f, 0.0f ) );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );

                bool open = true;
                if (ImGui::Begin(
                        "Stats", &open,
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
                    )) {

                    ImGui::SeparatorText( "RTTI" );
                    ImGui::Text( "Hits: %d", rtti::TypeRegistry::get_rtti_hits() );

                    ImGui::SeparatorText( "Rendering" );
                    const auto& stats = gfx->renderer->get_stats();
                    ImGui::Text( "GPU Duration: %.3f ms", stats.gpu_duration_ms );
                    ImGui::Text( "Input Assembler Primitives: %llu", stats.input_primitives );
                    ImGui::Text( "Input Assembler Vertices: %llu", stats.input_vertices );
                    ImGui::Text( "Vertex Shader Invocations: %llu", stats.vs_invocations );
                    ImGui::Text( "Pixel Shader Invocations: %llu", stats.ps_invocations );
                    ImGui::Text( "Clipping Primitives: %llu", stats.clipping_primitives );
                    ImGui::Text( "Clipping Invocations: %llu", stats.clipping_invocations );

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
                }

                ImGui::End();
                ImGui::PopStyleVar();
            }
        } );

    world.create_system( "EcsDebugFeature::Transform2DGizmos" )
        .with<EcsTransform2D>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto xform     = ctx.get_component<EcsTransform2D>();
            auto draw_list = ImGui::GetForegroundDrawList();
            auto center    = xform->get_world_center_point();
            char tmps[512];
            std::snprintf(
                tmps, 512, "Translation: X=%.3f Y=%.3f\nAngle: %.3f deg", static_cast<double>( xform->position.x ),
                static_cast<double>( xform->position.y ), static_cast<double>( xform->angle )
            );
            draw_list->AddText( ImVec2( xform->position.x, xform->position.y ), IM_COL32_BLACK, tmps );
            draw_list->AddCircleFilled( ImVec2( center.x, center.y ), 6, IM_COL32( 255, 0, 0, 255 ) );
        } );

    world.create_system( "EcsDebugFeature::TestSpinner" )
        .with<EcsTransform2D>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto xform = ctx.get_component<EcsTransform2D>();
            xform->angle += static_cast<float>( ctx.delta_time() );
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
            // auto gfx  = ctx.world().get_singleton<GraphicsModules>();
            // auto time = ctx.world().get_singleton<EcsTime>();

            // auto window = ctx.get_component<EcsWindow>();
            // if (time->accumulator >= 0.5) {
            //     update_window_title( gfx->window, id, window, time->fps, ctx.delta_time() );
            // }
        } );
}

} // namespace nc
