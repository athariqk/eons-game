#include "ecs_debug_feature.h"

#include <imgui.h>

#include <ncore/core/collection.h>
#include <ncore/core/quaternion.h>
#include <ncore/game_world.h>
#include <ncore/modules/io/resource_manager.h>
#include <ncore/modules/video/render_module.h>
#include <ncore/modules/video/renderer/vertex_format.h>
#include <ncore/resources/material_template.h>
#include <ncore/resources/mesh.h>
#include <ncore/runtime/components/ecs_events.h>
#include <ncore/runtime/components/ecs_material.h>
#include <ncore/runtime/components/ecs_mesh.h>
#include <ncore/runtime/components/ecs_resource.h>
#include <ncore/runtime/components/ecs_sprite.h>
#include <ncore/runtime/components/ecs_time.h>
#include <ncore/runtime/components/ecs_transform.h>
#include <ncore/runtime/components/ecs_window.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_world.h>

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
    float cube_rotation         = 0;
    bool cube_rot_switch        = true;
    Quaternion initial_cube_rot = Quaternion( 180, Vec3::up() );
    Quaternion target_cube_rot  = Quaternion( 0, Vec3::up() );
    float fov                   = 1.5708f;
    float near                  = 0.1f;
    float far                   = 100.0f;
    bool spin                   = true;
    NSTRUCT( DebugState, NC_F( DebugState, window_attrs ) )
};

void EcsDebugFeature::build( EcsWorld& world )
{
    world.emplace_singleton<DebugState>();

    world.observer( "EcsDebugFeature::CreateTestQuad" )
        .on<EcsSwapChainRef>( EcsCoreEvent::OnSet )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.world().get_singleton<IoModules>();
            auto sc = ctx.get_component<EcsSwapChainRef>();

            ctx.world()
                .entity( "TestQuad" )
                .with<EcsTransform2D>( { Vec2( sc->size.x / 2, sc->size.y / 2 ), Vec2( 150, 150 ) } )
                .with<EcsHasResource>()
                .with<EcsMaterialInstance>( { io->resources->load( "engine/materials/canvas.material" ) } )
                .with<EcsSpriteInstance>( { RID(), RID(), Color( 255, 125, 0, 255 ) } )
                .build();
        } );

    world.observer( "EcsDebugFeature::CreateTestMesh" )
        .on<EcsSwapChainRef>( EcsCoreEvent::OnSet )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.world().get_singleton<IoModules>();

            // clang-format off
			 Array<Vertex3D, 8> cube_verts = {
						//  px,    py,    pz,    nx,   ny,   nz,   tx,   ty,   tz,   tw,   u,    v,    u2,   v2,   color
			    Vertex3D{ -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF0000FF },
			    Vertex3D{ -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF00FF00 },
			    Vertex3D{  1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFF0000 },
			    Vertex3D{  1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			
			    Vertex3D{ -1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF00FFFF },
			    Vertex3D{ -1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFF00 },
			    Vertex3D{  1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFF00FF },
			    Vertex3D{  1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF333333 }
			};
			Array<uint16_t, 36> cube_indices = {
				2,0,1, 2,3,0,
				4,6,5, 4,7,6,
				0,7,4, 0,3,7,
				1,0,4, 1,4,5,
				1,5,2, 5,6,2,
				3,6,7, 3,2,6
			};
            // clang-format on

            auto mesh = Ref<Mesh>::create(
                MeshDesc{
                    .vertices = DynArray<std::byte>(
                        reinterpret_cast<std::byte const*>( cube_verts.data() ),
                        reinterpret_cast<std::byte const*>( cube_verts.data() + 8 )
                    ),
                    .indices       = DynArray<uint16_t>( cube_indices.data(), cube_indices.data() + 36 ),
                    .vertex_stride = sizeof( Vertex3D )
                }
            );
            auto mesh_rid = io->resources->add( mesh );

            ctx.world()
                .entity( "CubeMesh" )
                .with<EcsHasResource>()
                .with<EcsMeshInstance>( { mesh_rid, RID(), 5 } )
                .with<EcsMaterialInstance>( { io->resources->load( "engine/materials/pbr.material" ) } )
                .child_of( ctx.world()
                               .entity( "TestModel3D" )
                               .with<EcsTransform3D>( { Vec3( 0, 0, 0 ), Quaternion(), Vec3( 2, 2, 2 ) } )
                               .build() )
                .build();
        } );

    world.system( "EcsDebugFeature::DebugUI" )
        .with<EcsSwapChainRef>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto time  = ctx.world().get_singleton<EcsTime>();
            auto gfx   = ctx.world().get_singleton<GraphicsModules>();
            auto state = ctx.world().get_singleton<DebugState>();

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

                if (ImGui::Begin(
                        "Stats", nullptr,
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
                            .entity()
                            .with<EcsWindow>( EcsWindow{ .resolution = Vec2( 300, 300 ), .visible = true } )
                            .build();
                    }
                }
                ImGui::End();
                ImGui::PopStyleVar();
            }

            if (ImGui::Begin( "Camera" )) {
                ImGui::SeparatorText( "Camera" );

                if (ImGui::SliderAngle( "FoV", &state->fov, 60, 120, "%.3f deg" )) {
                    gfx->renderer->world_camera_set_fov( math::rad_to_deg( state->fov ) );
                }
                if (ImGui::SliderFloat( "Near", &state->near, 0.001f, 50.f )) {
                    gfx->renderer->world_camera_set_z_near( state->near );
                }
                if (ImGui::SliderFloat( "Far", &state->far, 50.f, 100.f )) {
                    gfx->renderer->world_camera_set_z_far( state->far );
                }

                if (ImGui::Button( "Reset" )) {
                    state->fov  = 1.5708f;
                    state->near = 0.1f;
                    state->far  = 100.0f;
                    gfx->renderer->world_camera_set_fov( math::rad_to_deg( state->fov ) );
                    gfx->renderer->world_camera_set_z_near( state->near );
                    gfx->renderer->world_camera_set_z_far( state->far );
                }
            }
            ImGui::End();

            if (ImGui::Begin( "Debug Stuff" )) {
                ImGui::Checkbox( "Test Spin", &state->spin );
            }
            ImGui::End();
        } );

    world.system( "EcsDebugFeature::Transform2DGizmos" )
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

    world.system( "EcsDebugFeature::TestSpinner" )
        .with<EcsTransform2D>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto state = ctx.world().get_singleton<DebugState>();
            auto xform = ctx.get_component<EcsTransform2D>();
            if (!state->spin)
                return;
            xform->angle += static_cast<float>( ctx.delta_time() ) * 3.5f;
        } );

    world.system( "EcsDebugFeature::TestSpinner3D" )
        .with<EcsTransform3D>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto state = ctx.world().get_singleton<DebugState>();
            auto xform = ctx.get_component<EcsTransform3D>();
            if (!state->spin)
                return;
            state->cube_rotation += static_cast<float>( ctx.delta_time() );
            if (state->cube_rotation >= 1.0f) {
                state->cube_rotation     = 0.0f;
                state->initial_cube_rot  = state->target_cube_rot;
                Quaternion flip_180      = Quaternion( 180, Vec3::up() );
                Quaternion weird_swaying = Quaternion( 30, Vec3::forward() );
                state->target_cube_rot   = state->initial_cube_rot * flip_180 * weird_swaying;
            }
            xform->rotation = Quaternion::slerp(
                state->initial_cube_rot, state->target_cube_rot, std::min( state->cube_rotation, 1.0f )
            );
        } );

#if !defined( NC_DIST )
    world.system( "EcsDebugFeature::HotReload" ).in( EcsSystemPhase::UPDATE ).run( []( QueryContext& ctx ) {
        auto io = ctx.world().get_singleton<IoModules>();

        if (ImGui::IsKeyPressed( ImGuiKey_F5 )) {
            NC_LOG_INFO_C( log::GRAPHICS, "Hot-reloading" );
            io->resources->load<MaterialTemplate>( "engine/shaders/pbr.slang", true );
            io->resources->load<MaterialTemplate>( "engine/materials/pbr.material", true );
            io->resources->load<MaterialTemplate>( "engine/shaders/canvas.slang", true );
            io->resources->load<MaterialTemplate>( "engine/materials/canvas.material", true );
        }
    } );
#endif

    // TODO: refactor this to use Timers
    world.system( "EcsDebugFeature::TitleBarUpdater" )
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
