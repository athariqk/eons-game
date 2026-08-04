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
#include <ncore/runtime/components/ecs_camera.h>
#include <ncore/runtime/components/ecs_events.h>
#include <ncore/runtime/components/ecs_input.h>
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
    NSTRUCT( DebugState, NC_F( DebugState, window_attrs ) )
};

struct TestSpin {
    float rotation   = 0;
    bool switch_rot  = true;
    Quaternion start = Quaternion( 180, Vec3::up() );
    Quaternion end   = Quaternion( 0, Vec3::up() );
    NSTRUCT(
        TestSpin, NC_F( TestSpin, rotation ) NC_F( TestSpin, switch_rot ) NC_F( TestSpin, start ) NC_F( TestSpin, end )
    )
};

void EcsDebugFeature::build( EcsWorld& world )
{
    world.emplace_singleton<DebugState>();

    world.system( "EcsDebugFeature::Init" ).in( EcsSystemPhase::INIT ).run( []( QueryContext& ctx ) {
        // TODO: make this into a prefab
        ctx.world()
            .entity( "FlyCam" )
            .with<EcsTransform3D>( { Vec3(), Quaternion::identity(), Vec3( 1, 1, 1 ) } )
            .with<EcsCamera>()
            .with<EcsInputReceiver>()
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
                    .vertices = DynamicArray<std::byte>(
                        reinterpret_cast<std::byte const*>( cube_verts.data() ),
                        reinterpret_cast<std::byte const*>( cube_verts.data() + 8 )
                    ),
                    .indices       = DynamicArray<uint16_t>( cube_indices.data(), cube_indices.data() + 36 ),
                    .vertex_stride = sizeof( Vertex3D )
                }
            );
            auto mesh_rid = io->resources->add( mesh );

            ctx.world()
                .entity( "CubeMesh" )
                .with<EcsHasResource>()
                .with<EcsMeshInstance>( { mesh_rid, RID(), 1 } )
                .with<EcsMaterialInstance>( { io->resources->load( "engine/materials/pbr.material" ) } )
                .child_of(
                    ctx.world()
                        .entity( "TestModel3D" )
                        .with<EcsTransform3D>( { Vec3( 0, 0, -10 ), Quaternion( 180, Vec3::up() ), Vec3( 1, 1, 1 ) } )
                        //.with<TestSpin>()
                        .build()
                )
                .build();
        } );

    world.observer( "EcsDebugFeature::SwapChainResizedDebug" )
        .with<EcsSwapChainRef>()
        .event<EcsSwapChainResized>()
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto resized = ctx.event_payload<EcsSwapChainResized>();
            NC_LOG_DEBUG_C(
                log::GRAPHICS, "SwapChainResized: size={}", rtti::TypeRegistry::to_string<Vec2>( &resized->size )
            );
        } );

    world.system( "EcsDebugFeature::DebugUI" )
        .with<EcsSwapChainRef>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto time = ctx.world().get_singleton<EcsTime>();
            auto gfx  = ctx.world().get_singleton<GraphicsModules>();
            auto sc   = ctx.get_component<EcsSwapChainRef>();

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu( "File" )) {
                    if (ImGui::MenuItem( "Quit", "Alt+F4" )) {
                        ctx.world().get_parent().request_quit();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            // Crosshair
            {
                auto draw_list = ImGui::GetBackgroundDrawList();
                ImVec2 center  = { sc->size.x * 0.5f, sc->size.y * 0.5f };
                draw_list->AddLine(
                    ImVec2( center.x - 15.0f, center.y ), ImVec2( center.x + 15.0f, center.y ),
                    IM_COL32( 255, 255, 255, 255 ), 1.5f
                );
                draw_list->AddLine(
                    ImVec2( center.x, center.y - 15.0f ), ImVec2( center.x, center.y + 15.0f ),
                    IM_COL32( 255, 255, 255, 255 ), 1.5f
                );
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

                if (ImGui::Begin( "Stats", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize )) {

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

                    char testbuf[100];
                    ImGui::InputText( "Test", testbuf, 100 );
                }
                ImGui::End();
                ImGui::PopStyleVar();
            }
        } );

    world.system( "EcsDebugFeature::3DCamDebugUI" )
        .with<EcsCamera, EcsTransform3D>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto gfx   = ctx.world().get_singleton<GraphicsModules>();
            auto cam   = ctx.get_component<EcsCamera>();
            auto xform = ctx.get_component<EcsTransform3D>();

            if (ImGui::Begin( "Camera" )) {
                if (ImGui::BeginTable( "Transform", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg )) {
                    for (int row = 0; row < 4; row++) {
                        ImGui::TableNextRow();
                        for (int col = 0; col < 4; col++) {
                            ImGui::TableSetColumnIndex( col );
                            ImGui::Text( "%.3f", static_cast<double>( xform->get_matrix().read( col, row ) ) );
                        }
                    }
                    ImGui::EndTable();
                }

                if (ImGui::SliderAngle( "FoV", &cam->fov, 60, 120 )) {
                    gfx->renderer->world_camera_set_fov( cam->fov );
                }
                if (ImGui::SliderFloat( "Near", &cam->z_near, 0.001f, 50.f )) {
                    gfx->renderer->world_camera_set_z_near( cam->z_near );
                }
                if (ImGui::SliderFloat( "Far", &cam->z_far, 50.f, 100.f )) {
                    gfx->renderer->world_camera_set_z_far( cam->z_far );
                }

                if (ImGui::Button( "Reset" )) {
                    cam->fov    = 1.5708f;
                    cam->z_near = 0.1f;
                    cam->z_far  = 100.0f;
                    gfx->renderer->world_camera_set_fov( cam->fov );
                    gfx->renderer->world_camera_set_z_near( cam->z_near );
                    gfx->renderer->world_camera_set_z_far( cam->z_far );
                    xform->translation = Vec3();
                    xform->rotation    = Quaternion::identity();
                }
            }
            ImGui::End();
        } );

    world.system( "EcsDebugFeature::TestSpinner" )
        .with<EcsTransform2D, TestSpin>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto xform = ctx.get_component<EcsTransform2D>();
            xform->angle += static_cast<float>( ctx.delta_time() ) * 3.5f;

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

    world.system( "EcsDebugFeature::TestSpinner3D" )
        .with<EcsTransform3D, TestSpin>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto xform = ctx.get_component<EcsTransform3D>();
            auto spin  = ctx.get_component<TestSpin>();
            spin->rotation += static_cast<float>( ctx.delta_time() );
            if (spin->rotation >= 1.0f) {
                spin->rotation           = 0.0f;
                spin->start              = spin->end;
                Quaternion flip_180      = Quaternion( 180, Vec3::up() );
                Quaternion weird_swaying = Quaternion( 30, Vec3::forward() );
                spin->end                = spin->start * flip_180 * weird_swaying;
            }
            xform->rotation = Quaternion::slerp( spin->start, spin->end, std::min( spin->rotation, 1.0f ) );
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
