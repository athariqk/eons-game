#include "ncore_editor.h"

#include <imgui_internal.h>

#include <ncore/core/quaternion.h>
#include <ncore/game_world.h>
#include <ncore/resources/material_template.h>
#include <ncore/runtime/components/camera.h>
#include <ncore/runtime/components/input.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/time.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/components/window.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/input/input_service.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/video/render_service.h>

#include "imgui_style.h"

namespace nc::editor {

struct EngineEditorState {
    Scene* current_scene = nullptr;
    EcsQuery spatial_query{};
    ImGuiID dockspace_id = 0;
    bool stats_window    = false;
    bool camera_window   = false;
    bool inputs_window   = false;
    Node* selected_node  = nullptr;
    NSTRUCT(
        EngineEditorState, NC_F( EngineEditorState, current_scene ) NC_F( EngineEditorState, spatial_query )
                               NC_F( EngineEditorState, dockspace_id ) NC_F( EngineEditorState, stats_window )
                                   NC_F( EngineEditorState, camera_window ) NC_F( EngineEditorState, inputs_window )
                                       NC_F( EngineEditorState, selected_node )
    )
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

static void draw_scene_tree_node( Node& node, EngineEditorState& state )
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (node.get_child_count() == 0) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (node == state.selected_node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    auto name = node.get_name();
    bool open =
        ImGui::TreeNodeEx( reinterpret_cast<void*>( node.get_id() ), flags, "%s##%llu", name.data(), node.get_id() );

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Button( "Delete" )) {
            node.destroy();
            state.selected_node = nullptr; // avoids crashing the ECS
        }
        ImGui::EndPopup();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state.selected_node = &node;
    }

    if (open) {
        for (auto& child : node.get_children()) {
            draw_scene_tree_node( child, state );
        }
        ImGui::TreePop();
    }
}

void register_editor_plugin( Scene& scene )
{
    auto editor_state           = scene.get_ecs().emplace_singleton<EngineEditorState>();
    editor_state->current_scene = &scene;

    scene.get_ecs()
        .system( "EngineEditorFeature::Init" )
        .with<EngineEditorState>()
        .in( EcsSystemPhase::INIT )
        .run( []( QueryContext& ctx ) {
            auto state = ctx.get_component<EngineEditorState>();

            // TODO: make this into a prefab
            ctx.world()
                .entity( "EditorFlyCam" )
                .with<Transform3DComponent>( { Vec3(), Quaternion::identity(), Vec3( 1, 1, 1 ) } )
                .with<CameraComponent>()
                .with<InputComponent>()
                .build();

            state->spatial_query =
                ctx.world().query( "EngineEditorFeature::InputReceiverDebugQuery" ).with<InputComponent>().build();
        } );

    scene.get_ecs()
        .observer( "EngineEditorFeature::InitStyling" )
        .on<GuiStateComponent>( EcsCoreEvent::OnSet )
        .run( []( QueryContext& ctx ) {
            StyleColorsEditor();
            StyleSizesEditor();
        } );

    scene.get_ecs()
        .observer( "EngineEditorFeature::SwapChainResizedDebug" )
        .with<SwapChainComponent>()
        .event<SwapChainResizedComponent>()
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto resized = ctx.event_payload<SwapChainResizedComponent>();
            NC_LOG_DEBUG_C(
                log::GRAPHICS, "SwapChainResized: size={}", rtti::TypeRegistry::to_string<Vec2>( &resized->size )
            );
        } );

    scene.get_ecs()
        .system( "EngineEditorFeature::ConfigureDocking" )
        .with<GuiStateComponent>()
        .with<EngineEditorState>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .run( []( QueryContext& ctx ) {
            auto state           = ctx.get_component<EngineEditorState>();
            ImGuiID dockspace_id = ImGui::DockSpaceOverViewport( 0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode );
            state->dockspace_id  = dockspace_id;
            if (!ImGui::DockBuilderGetNode( dockspace_id )) {
                ImGui::DockBuilderRemoveNode( dockspace_id );
                ImGui::DockBuilderAddNode( dockspace_id, ImGuiDockNodeFlags_DockSpace );
                ImGui::DockBuilderSetNodeSize( dockspace_id, ImGui::GetMainViewport()->WorkSize );
                ImGuiID dock_main = dockspace_id;
                ImGuiID dock_left = ImGui::DockBuilderSplitNode( dock_main, ImGuiDir_Left, 0.22f, nullptr, &dock_main );
                ImGui::DockBuilderDockWindow( "Scene Tree", dock_left );
                ImGui::DockBuilderFinish( dockspace_id );
            }
        } );

    scene.get_ecs()
        .system( "EngineEditorFeature::Panels" )
        .with<SwapChainComponent>()
        .with<EngineEditorState>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto time  = ctx.world().get_singleton<TimeComponent>();
            auto gfx   = ctx.world().get_singleton<GraphicsServices>();
            auto state = ctx.get_component<EngineEditorState>();
            auto sc    = ctx.get_component<SwapChainComponent>();

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu( "File" )) {
                    if (ImGui::MenuItem( "Quit", "Alt+F4" )) {
                        state->current_scene->request_quit();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu( "Debug" )) {
                    if (ImGui::MenuItem( "Stats" )) {
                        state->stats_window = true;
                    }
                    if (ImGui::MenuItem( "Camera" )) {
                        state->camera_window = true;
                    }
                    if (ImGui::MenuItem( "Inputs" )) {
                        state->inputs_window = true;
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
                if (ImGui::Begin( "Scene Tree" )) {
                    auto root = state->current_scene->root();
                    if (root) {
                        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
                        draw_scene_tree_node( *root, *state );
                    }
                }
                ImGui::End();
            }

            {
                if (ImGui::Begin( "Inspector" )) {
                    if (state->selected_node) {
                        auto name = state->selected_node->get_name();
                        auto id   = state->selected_node->get_id();

                        ImGui::Text( "Node: %s", name.data() );
                        ImGui::Text( "ID: %llu", id );
                        ImGui::Separator();

                        auto& ecs = state->current_scene->get_ecs();
                        for (auto comp_id : state->selected_node->get_components()) {
                            auto* type = ecs.resolve_component( comp_id );
                            if (!type)
                                continue;
                            if (type == rtti::TypeRegistry::find<NodeRefComponent>())
                                continue;

                            void* comp = state->selected_node->get_component( type );
                            if (!comp)
                                continue;

                            if (ImGui::CollapsingHeader( type->name, ImGuiTreeNodeFlags_DefaultOpen )) {
                                auto str = rtti::TypeRegistry::to_string( comp, type->id );
                                ImGui::TextUnformatted( str.c_str() );

                                ImGui::PushID( static_cast<int>( id << 32 | type->id.value ) );
                                if (ImGui::SmallButton( "Remove" )) {
                                    state->selected_node->remove_component( type );
                                }
                                ImGui::PopID();
                            }
                        }
                    } else {
                        ImGui::TextDisabled( "No node selected" );
                    }
                }
                ImGui::End();
            }

            {
                ImGui::SetNextWindowBgAlpha( 0.35f );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

                constexpr float PAD = 10.0f;

                float overlay_right_edge = work_pos.x + work_size.x;
                if (ImGuiDockNode* root = ImGui::DockBuilderGetNode( state->dockspace_id )) {
                    ImGuiDockNode* node = root;
                    while (node->IsSplitNode())
                        node = node->ChildNodes[1] ? node->ChildNodes[1] : node->ChildNodes[0];
                    if (!node->IsEmpty() && node->Pos.x > work_pos.x + 1.0f)
                        overlay_right_edge = node->Pos.x;
                }

                ImGui::SetNextWindowPos(
                    ImVec2( overlay_right_edge - PAD, work_pos.y + PAD ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f )
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

            if (state->stats_window) {
                if (ImGui::Begin( "Stats", &state->stats_window )) {

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
                            .with<WindowComponent>( WindowComponent{ .resolution = Vec2( 300, 300 ), .visible = true } )
                            .build();
                    }
                }
                ImGui::End();
            }
        } );

    scene.get_ecs()
        .system( "EngineEditorFeature::CameraPanel" )
        .with<CameraComponent, Transform3DComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto state = ctx.world().get_singleton<EngineEditorState>();
            if (!state->camera_window)
                return;

            auto gfx   = ctx.world().get_singleton<GraphicsServices>();
            auto cam   = ctx.get_component<CameraComponent>();
            auto xform = ctx.get_component<Transform3DComponent>();

            if (ImGui::Begin( "Camera", &state->camera_window )) {
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

    scene.get_ecs()
        .system( "EngineEditorFeature::TestSpinner" )
        .with<Transform2DComponent, TestSpin>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto xform = ctx.get_component<Transform2DComponent>();
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

    scene.get_ecs()
        .system( "EngineEditorFeature::TestSpinner3D" )
        .with<Transform3DComponent, TestSpin>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto xform = ctx.get_component<Transform3DComponent>();
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
    scene.get_ecs()
        .system( "EngineEditorFeature::HotReload" )
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.world().get_singleton<IoServices>();

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
    scene.get_ecs()
        .system( "EngineEditorFeature::TitleBarUpdater" )
        .with<WindowComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .each( []( QueryContext& ctx, EcsEntity id ) {
            // auto gfx  = ctx.world().get_singleton<GraphicsServices>();
            // auto time = ctx.world().get_singleton<TimeComponent>();

            // auto window = ctx.get_component<WindowComponent>();
            // if (time->accumulator >= 0.5) {
            //     update_window_title( gfx->window, id, window, time->fps, ctx.delta_time() );
            // }
        } );

    scene.get_ecs()
        .system( "EngineEditorFeature::InputUI" )
        .with<IoServices>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto state = ctx.world().get_singleton<EngineEditorState>();
            if (!state->inputs_window)
                return;

            auto io = ctx.world().get_singleton<IoServices>();

            if (ImGui::Begin( "Input Debug", &state->inputs_window )) {
                {
                    ImGui::SeparatorText( "Actions" );
                    DynamicArray<StringView> actions;
                    io->inputs->action_list( actions );

                    if (ImGui::BeginTable( "ActionsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg )) {
                        ImGui::TableSetupColumn( "Name" );
                        ImGui::TableSetupColumn( "Held" );
                        ImGui::TableSetupColumn( "Pressed" );
                        ImGui::TableSetupColumn( "Released" );
                        ImGui::TableHeadersRow();

                        for (const auto& action : actions) {
                            bool held     = io->inputs->action_is_held( action.data() );
                            bool pressed  = io->inputs->action_is_pressed( action.data() );
                            bool released = io->inputs->action_is_released( action.data() );

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex( 0 );
                            ImGui::Text( "%s", action.data() );
                            ImGui::TableSetColumnIndex( 1 );
                            ImGui::TextColored(
                                held ? ImVec4( 0.4f, 1.0f, 0.4f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                held ? "Yes" : "No"
                            );
                            ImGui::TableSetColumnIndex( 2 );
                            ImGui::TextColored(
                                pressed ? ImVec4( 1.0f, 1.0f, 0.3f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                pressed ? "Yes" : "No"
                            );
                            ImGui::TableSetColumnIndex( 3 );
                            ImGui::TextColored(
                                released ? ImVec4( 1.0f, 0.6f, 0.2f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                released ? "Yes" : "No"
                            );
                        }
                        ImGui::EndTable();
                    }
                }

                {
                    ImGui::SeparatorText( "Mouse Input" );
                    ImGui::DragFloat2(
                        "Pos", io->inputs->get_mouse_position().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                    ImGui::DragFloat2(
                        "Delta", io->inputs->get_mouse_delta().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                    ImGui::DragFloat2(
                        "Wheel", io->inputs->get_mouse_wheel().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                }

                {
                    ImGui::SeparatorText( "Input Receivers" );
                    for (auto iter = state->spatial_query.begin(); iter != nullptr; ++iter) {
                        QueryContext qctx( iter.get_internal_iter() );
                        for (int32_t row = 0; row < qctx.count(); row++) {
                            qctx.set_row( row );
                            auto input = qctx.get_component<InputComponent>();
                            ImGui::Text(
                                "EID %llu | dir (%.2f, %.2f, %.2f) | mag %.2f | orient (%.2f, %.2f, %.2f)",
                                static_cast<unsigned long long>( qctx.entity( row ) ),
                                static_cast<double>( input->direction.x ), static_cast<double>( input->direction.y ),
                                static_cast<double>( input->direction.z ), static_cast<double>( input->magnitude ),
                                static_cast<double>( input->angular_delta.x ),
                                static_cast<double>( input->angular_delta.y ),
                                static_cast<double>( input->angular_delta.z )
                            );
                        }
                    }
                }
            }
            ImGui::End();
        } );
}

} // namespace nc::editor
