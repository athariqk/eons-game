// clang-format off
#include <imgui_internal.h>
#include <ImGuizmo.h> // must come after imgui includes
// clang-format on

#include <editor/ncore_editor.h>
#include <ncore/core/color.h>
#include <ncore/core/quaternion.h>
#include <ncore/core/rid.h>
#include <ncore/game_world.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/time.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/components/window.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>

#include "editor_camera.h"
#include "editor_state.h"
#include "gui_plugin.h"

namespace nc::editor {

static void draw_scene_tree_node( Node& node, EditorState& state )
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull;
    if (node.get_child_count() == 0) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (node == state.SelectedNode) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    auto name = node.get_name();
    bool open = ImGui::TreeNodeEx( reinterpret_cast<void*>( node.get_id() ), flags, "%s", name.data() );

    if (ImGui::BeginDragDropSource( ImGuiDragDropFlags_None )) {
        Node* ptr = &node;
        ImGui::SetDragDropPayload( "SCENE_TREE_NODE", &ptr, sizeof( ptr ) );
        ImGui::Text( "%s", name.data() );
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "SCENE_TREE_NODE" )) {
            Node* dragged = *static_cast<Node**>( payload->Data );
            if (dragged && dragged != &node) {
                dragged->reparent_to( &node );
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Button( "Rename" )) {
            auto n    = node.get_name();
            auto nlen = std::min( n.size(), sizeof( state.NodeRenameBuf ) - 1 );
            std::memcpy( state.NodeRenameBuf, n.data(), nlen );
            state.NodeRenameBuf[nlen] = '\0';
            state.NodeToRename        = &node;
            state.ShowRenamePopup     = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button( "Delete" )) {
            node.destroy();
            state.SelectedNode = nullptr; // avoids crashing the ECS
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state.SelectedNode = &node;
    }

    if (open) {
        for (auto& child : node.get_children()) {
            draw_scene_tree_node( child, state );
        }
        ImGui::TreePop();
    }
}

using WidgetDrawFn = void ( * )( const char* label, void* ptr, bool editable );

static void draw_float_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::DragFloat( label, static_cast<float*>( ptr ), 0.01f );
    else
        ImGui::Text( "%s: %.3f", label, static_cast<double>( *static_cast<float*>( ptr ) ) );
}

static void draw_double_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::InputDouble( label, static_cast<double*>( ptr ) );
    else
        ImGui::Text( "%s: %.3f", label, *static_cast<double*>( ptr ) );
}

static void draw_bool_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::Checkbox( label, static_cast<bool*>( ptr ) );
    else
        ImGui::Text( "%s: %s", label, *static_cast<bool*>( ptr ) ? "true" : "false" );
}

static void draw_int32_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::DragInt( label, static_cast<int*>( ptr ), 0.1f );
    else
        ImGui::Text( "%s: %d", label, *static_cast<int*>( ptr ) );
}

static void draw_uint32_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::DragScalar( label, ImGuiDataType_U32, ptr, 0.1f );
    else
        ImGui::Text( "%s: %u", label, *static_cast<uint32_t*>( ptr ) );
}

static void draw_uint8_widget( const char* label, void* ptr, bool editable )
{
    if (editable) {
        int val = *static_cast<uint8_t*>( ptr );
        if (ImGui::DragInt( label, &val, 0.1f, 0, 255 ))
            *static_cast<uint8_t*>( ptr ) = static_cast<uint8_t>( std::clamp( val, 0, 255 ) );
    } else {
        ImGui::Text( "%s: %u", label, *static_cast<uint8_t*>( ptr ) );
    }
}

static void draw_vec2f_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::DragFloat2( label, static_cast<Vec2f*>( ptr )->data(), 0.01f );
    else {
        auto* v = static_cast<Vec2f*>( ptr );
        ImGui::Text( "%s: (%.2f, %.2f)", label, static_cast<double>( v->x ), static_cast<double>( v->y ) );
    }
}

static void draw_vec2i_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::DragInt2( label, static_cast<Vec2i*>( ptr )->data(), 0.01f );
    else {
        auto* v = static_cast<Vec2i*>( ptr );
        ImGui::Text( "%s: (%d, %d)", label, v->x, v->y );
    }
}

static void draw_vec3_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::DragFloat3( label, static_cast<Vec3*>( ptr )->data(), 0.01f );
    else {
        auto* v = static_cast<Vec3*>( ptr );
        ImGui::Text(
            "%s: (%.2f, %.2f, %.2f)", label, static_cast<double>( v->x ), static_cast<double>( v->y ),
            static_cast<double>( v->z )
        );
    }
}

static void draw_vec4_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::DragFloat4( label, static_cast<Vec4*>( ptr )->data(), 0.01f );
    else {
        auto* v = static_cast<Vec4*>( ptr );
        ImGui::Text(
            "%s: (%.2f, %.2f, %.2f, %.2f)", label, static_cast<double>( v->x ), static_cast<double>( v->y ),
            static_cast<double>( v->z ), static_cast<double>( v->w )
        );
    }
}

static void draw_color_widget( const char* label, void* ptr, bool editable )
{
    auto* c     = static_cast<Color*>( ptr );
    float col[] = { c->r / 255.0f, c->g / 255.0f, c->b / 255.0f, c->a / 255.0f };
    if (editable) {
        if (ImGui::ColorEdit4( label, col )) {
            c->r = static_cast<uint8_t>( std::clamp( col[0] * 255.0f, 0.0f, 255.0f ) );
            c->g = static_cast<uint8_t>( std::clamp( col[1] * 255.0f, 0.0f, 255.0f ) );
            c->b = static_cast<uint8_t>( std::clamp( col[2] * 255.0f, 0.0f, 255.0f ) );
            c->a = static_cast<uint8_t>( std::clamp( col[3] * 255.0f, 0.0f, 255.0f ) );
        }
    } else {
        ImGui::ColorButton(
            label, ImVec4( col[0], col[1], col[2], col[3] ), ImGuiColorEditFlags_NoTooltip, ImVec2( 20, 20 )
        );
        ImGui::SameLine();
        ImGui::Text( "%s", label );
    }
}

static void draw_quaternion_widget( const char* label, void* ptr, bool editable )
{
    auto* q      = static_cast<Quaternion*>( ptr );
    float vals[] = { q->w, q->v.x, q->v.y, q->v.z };
    if (editable) {
        if (ImGui::DragFloat4( label, vals, 0.01f, -1.0f, 1.0f )) {
            q->w   = vals[0];
            q->v.x = vals[1];
            q->v.y = vals[2];
            q->v.z = vals[3];
        }
    } else {
        ImGui::Text(
            "%s: (%.2f, %.2f, %.2f, %.2f)", label, static_cast<double>( vals[0] ), static_cast<double>( vals[1] ),
            static_cast<double>( vals[2] ), static_cast<double>( vals[3] )
        );
    }
}

static void draw_rid_widget( const char* label, void* ptr, bool )
{
    ImGui::Text( "%s: 0x%016llx", label, static_cast<RID*>( ptr )->value );
}

static const WidgetDrawFn* find_widget( rtti::TypeId id )
{
    struct Binding {
        rtti::TypeId type_id;
        WidgetDrawFn draw;
    };
    static const Binding table[] = {
        { rtti::TypeRegistry::find<float>()->id, draw_float_widget },
        { rtti::TypeRegistry::find<double>()->id, draw_double_widget },
        { rtti::TypeRegistry::find<bool>()->id, draw_bool_widget },
        { rtti::TypeRegistry::find<int>()->id, draw_int32_widget },
        { rtti::TypeRegistry::find<int32_t>()->id, draw_int32_widget },
        { rtti::TypeRegistry::find<uint32_t>()->id, draw_uint32_widget },
        { rtti::TypeRegistry::find<uint8_t>()->id, draw_uint8_widget },
        { rtti::TypeRegistry::find<Vec2f>()->id, draw_vec2f_widget },
        { rtti::TypeRegistry::find<Vec2i>()->id, draw_vec2i_widget },
        { rtti::TypeRegistry::find<Vec3>()->id, draw_vec3_widget },
        { rtti::TypeRegistry::find<Vec4>()->id, draw_vec4_widget },
        { rtti::TypeRegistry::find<Color>()->id, draw_color_widget },
        { rtti::TypeRegistry::find<Quaternion>()->id, draw_quaternion_widget },
        { rtti::TypeRegistry::find<RID>()->id, draw_rid_widget },
    };
    for (auto& entry : table) {
        if (entry.type_id == id)
            return &entry.draw;
    }
    return nullptr;
}

// Returns true if the field has been edited by interaction.
static bool draw_field_widget( void* instance, const rtti::FieldInfo& field )
{
    auto type = field.get_type();
    if (!type)
        return false;

    void* ptr = field.get_void_ptr( instance );
    if (!ptr)
        return false;

    bool is_editable  = field.is( rtti::PropertyFlags::EDITABLE ) && !field.is( rtti::PropertyFlags::READ_ONLY );
    const char* label = field.name.data();

    if (field.qualifier.is_cstring) {
        ImGui::Text( "%s: \"%s\"", label, *static_cast<const char* const*>( ptr ) );
        return false;
    }
    if (field.qualifier.is_pointer()) {
        ImGui::Text( "%s: %p", label, *static_cast<const void* const*>( ptr ) );
        return false;
    }
    if (field.qualifier.is_array()) {
        ImGui::Text( "%s: [array x%d]", label, static_cast<int>( field.qualifier.array_length ) );
        return false;
    }

    auto* draw = find_widget( field.type_id );
    if (draw) {
        ( *draw )( label, ptr, is_editable );
        return false;
    }

    bool edited = false;
    switch (type->kind) {
        case rtti::TypeKind::STRING: {
            String str;
            type->to_string( str, ptr );
            ImGui::Text( "%s: %s", label, str.c_str() );
            break;
        }
        case rtti::TypeKind::ENUM: {
            auto* enum_t = static_cast<const rtti::EnumInfo*>( type );
            auto cur_val = enum_t->get_value( ptr );
            if (ImGui::BeginCombo( label, enum_t->get_name( cur_val ).data() )) {
                for (auto& elem : enum_t->elements()) {
                    const bool is_selected = ( cur_val == elem.value );
                    if (is_editable && ImGui::Selectable( elem.name.data(), is_selected )) {
                        enum_t->set_value( ptr, elem.value );
                        edited = true;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            break;
        }
        case rtti::TypeKind::RECORD:
        case rtti::TypeKind::VECTOR: {
            auto* record = static_cast<const rtti::RecordInfo*>( type );
            if (ImGui::TreeNode( label )) {
                for (auto& sub_field : record->fields()) {
                    ImGui::PushID( sub_field.name.data() );
                    draw_field_widget( ptr, sub_field );
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            break;
        }
        default:
            ImGui::Text( "%s: <?>", label );
            break;
    }

    return edited;
}

void register_editor_plugin( Scene& scene )
{
    register_gui_plugin( scene );
    register_editor_camera( scene );

    auto editor_state          = scene.get_ecs().add_singleton<EditorState>();
    editor_state->CurrentScene = &scene;

    editor_state->LogsListenerToken = log::add_listener( [editor_state]( const log::LogMsg& msg ) {
        if (editor_state->LogsOffset.empty()) {
            editor_state->LogsOffset.push_back( 0 );
        }

        int old_size = editor_state->LogsBuffer.size();
        editor_state->LogsBuffer.appendf( msg.payload.c_str(), msg.payload.c_str() + msg.payload.size() );
        editor_state->LogsBuffer.append( "\n" );

        for (int i = old_size; i < editor_state->LogsBuffer.size(); i++) {
            if (editor_state->LogsBuffer[i] == '\n') {
                editor_state->LogsOffset.push_back( i + 1 );
            }
        }
    } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_Init" )
        .with<EditorState>()
        .with<GuiStateComponent>()
        .in( EcsSystemPhase::INIT )
        .order( 20 )
        .run( []( EcsIterState& it ) {
            auto gui = it.get_component<GuiStateComponent>();
            ImGuizmo::SetImGuiContext( gui->ImGuiCtx );
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_BeginFrame" )
        .with<EditorState>()
        .with<GuiStateComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .run( []( EcsIterState& it ) {
            auto state = it.get_component<EditorState>();

            ImGuizmo::SetOrthographic( false );
            ImGuizmo::BeginFrame();

            // auto view_matrix = vid->Gfx->world_get_view_matrix().data();
            // auto proj_matrix = vid->Gfx->world_camera_get_projection().data();

            // ImGuizmo's DrawGrid is verrry buggy
            // Mat4 grid_matrix;
            // ImGuizmo::RecomposeMatrixFromComponents(
            //    state->GridPos.data(), state->GridRotation.data(), state->GridScale.data(), grid_matrix.data()
            //);
            // ImGuizmo::DrawGrid( view_matrix, proj_matrix, grid_matrix.data(), 100.0f );

            // Crosshair
            /*if (state->ViewportRT) {
                auto draw_list = ImGui::GetBackgroundDrawList();
                ImVec2 center  = { state->ViewportSize.x * 0.5f, state->ViewportSize.y * 0.5f };
                draw_list->AddLine(
                    ImVec2( center.x - 15.0f, center.y ), ImVec2( center.x + 15.0f, center.y ),
                    IM_COL32( 255, 255, 255, 255 ), 1.5f
                );
                draw_list->AddLine(
                    ImVec2( center.x, center.y - 15.0f ), ImVec2( center.x, center.y + 15.0f ),
                    IM_COL32( 255, 255, 255, 255 ), 1.5f
                );
            }*/
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_CanvasSwapchainPass" )
        .with<EditorState>()
        .with<VideoServices>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 15 )
        .run( []( EcsIterState& it ) {
            auto state = it.get_component<EditorState>();
            auto vid   = it.get_component<VideoServices>();

            RenderService::RenderPassDesc pass{};
            pass.camera       = state->EditorCamSource;
            pass.draw_spatial = false;
            pass.draw_canvas  = true;
            pass.to_screen    = true;
            vid->Gfx->render_pass( pass );
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_ConfigureDocking" )
        .with<GuiStateComponent>()
        .with<EditorState>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .run( []( EcsIterState& it ) {
            auto state           = it.get_component<EditorState>();
            ImGuiID dockspace_id = ImGui::DockSpaceOverViewport( 0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode );
            state->DockspaceId   = dockspace_id;
            if (!ImGui::DockBuilderGetNode( dockspace_id )) {
                ImGui::DockBuilderRemoveNode( dockspace_id );
                ImGui::DockBuilderAddNode( dockspace_id, ImGuiDockNodeFlags_DockSpace );
                ImGui::DockBuilderSetNodeSize( dockspace_id, ImGui::GetMainViewport()->WorkSize );
                ImGuiID dock_main = dockspace_id;
                ImGuiID dock_top  = ImGui::DockBuilderSplitNode( dock_main, ImGuiDir_Up, 0.05f, nullptr, &dock_main );
                ImGuiID dock_left = ImGui::DockBuilderSplitNode( dock_main, ImGuiDir_Left, 0.22f, nullptr, &dock_main );
                ImGuiID dock_right =
                    ImGui::DockBuilderSplitNode( dock_main, ImGuiDir_Right, 0.22f, nullptr, &dock_main );
                ImGui::DockBuilderDockWindow( "Toolbar", dock_top );
                ImGui::DockBuilderDockWindow( "Scene Tree", dock_left );
                ImGui::DockBuilderDockWindow( "Scene View", dock_main );
                ImGui::DockBuilderDockWindow( "Inspector", dock_right );
                ImGui::DockBuilderFinish( dockspace_id );
            }
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_SceneView" )
        .with<EditorState>()
        .with<VideoServices>()
        .with<TimeComponent>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( EcsIterState& it ) {
            auto state = it.get_component<EditorState>();
            auto vid   = it.get_component<VideoServices>();
            auto time  = it.get_component<TimeComponent>();

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
            ImGui::Begin( "Scene View", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

            state->ViewportHovered = ImGui::IsWindowHovered();
            state->ViewportFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_RootAndChildWindows );

            ImVec2 img_size = ImGui::GetContentRegionAvail();
            ImVec2 img_pos  = ImGui::GetCursorScreenPos();
            Vec2f img_size_v( img_size.x, img_size.y );

            // re-create viewport render target everytime size changed
            if (img_size.x > 0 && img_size.y > 0 && img_size_v != state->ViewportSize) {
                if (state->ViewportRT)
                    vid->Gfx->destroy_rid( state->ViewportRT );
                if (state->ViewportDT)
                    vid->Gfx->destroy_rid( state->ViewportDT );

                Vec2i vp_size( static_cast<int>( img_size.x ), static_cast<int>( img_size.y ) );
                state->ViewportRT   = vid->Gfx->texture_render_create( vp_size, TextureFormat::RGBA8_UNORM_SRGB );
                state->ViewportDT   = vid->Gfx->texture_render_create( vp_size, TextureFormat::D32_FLOAT );
                state->ViewportSize = img_size_v;
            }

            if (state->ViewportRT) {
                ImGuizmo::SetRect( img_pos.x, img_pos.y, img_size.x, img_size.y );

                ImTextureID tex_id = reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( state->ViewportRT.value ) );
                ImGui::Image( tex_id, img_size );
            }

            // -- Gizmos --

            // this means we only support single node selection
            // TODO: node multi-selection
            auto selected = state->SelectedNode;
            if (selected && selected->has_component<Transform3DComponent>()) {
                auto xform = selected->get_component<Transform3DComponent>();

                ImGuizmo::SetDrawlist( ImGui::GetWindowDrawList() );

                auto& cam_attribs   = vid->Gfx->camera_get_attribs( state->EditorCamSource );
                auto projection_mat = vid->Gfx->camera_get_perspective( state->EditorCamSource );
                auto view_mat       = cam_attribs.Transform.affine_inverse();

                Mat4 local     = xform->to_matrix();
                auto mode      = state->GlobalXformGizmo ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL;
                auto operation = static_cast<ImGuizmo::OPERATION>( state->XformGizmoOperation );
                if (ImGuizmo::Manipulate( view_mat.data(), projection_mat.data(), operation, mode, local.data() )) {
                    xform->from_matrix( local );
                }
            }

            ImGui::End();
            ImGui::PopStyleVar( 2 );

            // -- Overlays --

            ImGui::SetNextWindowBgAlpha( 0.35f );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

            constexpr float PAD      = 10.0f;
            float overlay_right_edge = img_pos.x + img_size.x;
            ImGui::SetNextWindowPos(
                ImVec2( overlay_right_edge - PAD, img_pos.y + PAD ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f )
            );

            ImGui::SetNextWindowSize( ImVec2( 200, 0 ) );

            if (ImGui::Begin( "##overlay", nullptr, window_flags )) {
                ImGui::Text( "Ticks: %u", time->Ticks );
                ImGui::Text( "FPS: %.3f", time->FPS );
                ImGui::Text( "Frame count: %d", time->FrameCount );

                const auto& stats = vid->Gfx->get_stats();
                ImGui::Text( "GPU time: %.3f ms", stats.gpu_duration_ms );
                ImGui::Text( "IA Prims: %llu", stats.input_primitives );
                ImGui::Text( "IA Verts: %llu", stats.input_vertices );
                ImGui::Text( "VS Invokes: %llu", stats.vs_invocations );
                ImGui::Text( "PS Invokes: %llu", stats.ps_invocations );
                ImGui::Text( "Clipping Prims: %llu", stats.clipping_primitives );
                ImGui::Text( "Clipping Invokes: %llu", stats.clipping_invocations );
            }
            ImGui::End();
            ImGui::PopStyleVar();
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_GameView" )
        .with<EditorState>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( EcsIterState& it ) {
            auto state = it.get_component<EditorState>();

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
            ImGui::Begin( "Game View", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

            if (state->GameViewRT) {
                ImVec2 img_size    = ImGui::GetContentRegionAvail();
                ImTextureID tex_id = reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( state->GameViewRT.value ) );
                ImGui::Image( tex_id, img_size );
            }

            ImGui::End();
            ImGui::PopStyleVar( 2 );
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_Panels" )
        .with<EditorState>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( EcsIterState& it ) {
            auto time  = it.world().get_singleton<TimeComponent>();
            auto vid   = it.world().get_singleton<VideoServices>();
            auto state = it.get_component<EditorState>();

            // Main Menu Bar
            {
                if (ImGui::BeginMainMenuBar()) {
                    if (ImGui::BeginMenu( "File" )) {
                        if (ImGui::MenuItem( "Quit", "Alt+F4" )) {
                            state->CurrentScene->request_quit();
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu( "Debug" )) {
                        if (ImGui::MenuItem( "Logs" )) {
                            state->ShowLogsWindow = true;
                        }
                        if (ImGui::MenuItem( "Stats" )) {
                            state->ShowStatsWindow = true;
                        }
                        if (ImGui::MenuItem( "Inputs" )) {
                            state->ShowInputsWindow = true;
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMainMenuBar();
                }
            }

            // Toolbar
            {
                ImGuiWindowClass window_class;
                window_class.DockingAllowUnclassed = true;
                window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;
                window_class.DockNodeFlagsOverrideSet |=
                    ImGuiDockNodeFlags_HiddenTabBar; // ImGuiDockNodeFlags_NoTabBar // FIXME: Will need a working Undock
                                                     // widget for _NoTabBar to work
                window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingSplit;
                window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverMe;
                window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverOther;
                ImGui::SetNextWindowClass( &window_class );

                auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
                if (ImGui::Begin( "Toolbar", nullptr, flags )) {
                    if (ImGui::Button( "Run" )) {
                        /* TODO */
                    }
                    ImGui::SameLine();
                    ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
                    ImGui::SameLine();
                    ImGui::Checkbox( "World", &state->GlobalXformGizmo );
                    ImGui::SameLine();
                    if (ImGui::RadioButton(
                            "Universal",
                            state->XformGizmoOperation == static_cast<int>( ImGuizmo::OPERATION::UNIVERSAL )
                        ))
                        state->XformGizmoOperation = static_cast<int>( ImGuizmo::OPERATION::UNIVERSAL );
                    ImGui::SameLine();
                    if (ImGui::RadioButton(
                            "Translate",
                            state->XformGizmoOperation == static_cast<int>( ImGuizmo::OPERATION::TRANSLATE )
                        ))
                        state->XformGizmoOperation = static_cast<int>( ImGuizmo::OPERATION::TRANSLATE );
                    ImGui::SameLine();
                    if (ImGui::RadioButton(
                            "Rotate", state->XformGizmoOperation == static_cast<int>( ImGuizmo::OPERATION::ROTATE )
                        ))
                        state->XformGizmoOperation = static_cast<int>( ImGuizmo::OPERATION::ROTATE );
                    ImGui::SameLine();
                    if (ImGui::RadioButton(
                            "Scale", state->XformGizmoOperation == static_cast<int>( ImGuizmo::OPERATION::SCALE )
                        ))
                        state->XformGizmoOperation = static_cast<int>( ImGuizmo::OPERATION::SCALE );
                    ImGui::SameLine();
                    ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
                    ImGui::SameLine();
                    if (ImGui::Checkbox( "Wireframe", &state->DrawWireframe )) {
                        auto q = it.world().query( "MaterialComponentOwners" ).with<MaterialComponent>().build();
                        for (auto it : q.entities()) {
                            auto mat      = it.get_component<MaterialComponent>();
                            mat->DrawMode = state->DrawWireframe ? FillMode::WIREFRAME : FillMode::SOLID;
                            it.mark_component_modified<MaterialComponent>();
                        }
                    }
                }
                ImGui::End();
            }

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 work_pos               = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
            ImVec2 work_size              = viewport->WorkSize;

            // Left panels
            {
                auto root = state->CurrentScene->root();

                if (ImGui::Begin( "Scene Tree" )) {
                    if (root) {
                        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
                        draw_scene_tree_node( *root, *state );
                    }

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( "SCENE_TREE_NODE" )) {
                            Node* dragged = *static_cast<Node**>( payload->Data );
                            if (dragged && root) {
                                dragged->reparent_to( root );
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if (ImGui::BeginPopupContextWindow( "Scene Tree Context Menu", ImGuiPopupFlags_NoOpenOverItems )) {
                        if (ImGui::Button( "Spawn Entity" )) {
                            root->create_child();
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::End();

                if (state->ShowRenamePopup) {
                    ImGui::OpenPopup( "Entity Rename" );
                    state->ShowRenamePopup = false;
                }

                if (ImGui::BeginPopupModal( "Entity Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize )) {
                    ImGui::InputText(
                        "##rename", state->NodeRenameBuf, sizeof( state->NodeRenameBuf ),
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll
                    );
                    bool confirmed = ImGui::Button( "OK" ) || ImGui::IsKeyPressed( ImGuiKey_Enter );
                    if (confirmed && state->NodeToRename && state->NodeRenameBuf[0]) {
                        state->NodeToRename->set_name( state->NodeRenameBuf );
                        state->NodeToRename = nullptr;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button( "Cancel" )) {
                        state->NodeToRename = nullptr;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }

            // Right panels
            {
                auto& ecs = state->CurrentScene->get_ecs();

                if (ImGui::Begin( "Inspector" )) {
                    if (state->SelectedNode) {
                        auto name   = state->SelectedNode->get_name();
                        auto id     = state->SelectedNode->get_id();
                        auto active = state->SelectedNode->get_active();

                        ImGui::Text( "Node: %s", name.data() );
                        ImGui::Text( "ID: %llu", id );
                        ImGui::Checkbox( "Active", active );
                        ImGui::Separator();

                        if (ImGui::Button( "+ Add Component", ImVec2( -FLT_MIN, 0 ) ))
                            ImGui::OpenPopup( "AddComponent" );

                        if (ImGui::BeginPopup( "AddComponent" )) {
                            ImGui::InputTextWithHint(
                                "##filter", "Search...", state->AddCompFilter, sizeof( state->AddCompFilter )
                            );

                            ImGui::BeginChild( "##complist", ImVec2( 0, 200 ), ImGuiChildFlags_Borders );
                            for (auto type : ecs.get_component_types()) {
                                if (!type->is_record())
                                    continue;
                                if (type == rtti::TypeRegistry::find<NodeRefComponent>())
                                    continue;
                                if (state->AddCompFilter[0] && !strstr( type->name, state->AddCompFilter ))
                                    continue;

                                bool already = state->SelectedNode->has_component( type );
                                ImGui::BeginDisabled( already );
                                if (ImGui::Selectable( type->name )) {
                                    state->SelectedNode->add_component( type );
                                    state->AddCompFilter[0] = '\0';
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::EndDisabled();
                            }
                            ImGui::EndChild();
                            ImGui::EndPopup();
                        }

                        for (auto& comp : state->SelectedNode->get_components()) {
                            auto* type = ecs.resolve_component( comp.EcsId );
                            if (!type)
                                continue;
                            if (type == rtti::TypeRegistry::find<NodeRefComponent>())
                                continue;

                            void* comp_data = state->SelectedNode->get_component( type );
                            if (!comp_data)
                                continue;

                            ImGui::Separator();
                            if (ImGui::CollapsingHeader( type->name, ImGuiTreeNodeFlags_DefaultOpen )) {
                                if (type->is_record()) {
                                    auto* record = static_cast<const rtti::RecordInfo*>( type );
                                    for (auto& field : record->fields()) {
                                        ImGui::PushID( field.name.data() );
                                        if (draw_field_widget( comp_data, field )) {
                                            state->SelectedNode->mark_component_modified( type );
                                        }
                                        ImGui::PopID();
                                    }
                                }

                                ImGui::PushID( static_cast<int>( id << 16 | type->id.value ) );
                                if (ImGui::SmallButton( "Remove" )) {
                                    state->SelectedNode->remove_component( type );
                                }
                                ImGui::BeginDisabled( !comp.Toggleable );
                                ImGui::Checkbox( "Active", &comp.Active );
                                ImGui::EndDisabled();
                                ImGui::PopID();
                            }
                        }
                    } else {
                        ImGui::TextDisabled( "No node selected" );
                    }
                }
                ImGui::End();
            }

            // Debug console
            if (state->ShowLogsWindow) {
                if (ImGui::Begin( "Logs", &state->ShowLogsWindow )) {
                    if (ImGui::SmallButton( "Clear" )) {
                        state->LogsBuffer.clear();
                        state->LogsOffset.clear();
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton( "Copy" ))
                        ImGui::SetClipboardText( state->LogsBuffer.c_str() );
                    ImGui::SameLine();
                    if (ImGui::SmallButton( "Say Hello World" ))
                        NC_LOG_INFO( "Hello World" );

                    ImGui::BeginChild(
                        "##log", ImVec2( 0.0f, 0.0f ), ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar
                    );

                    const char* buf     = state->LogsBuffer.begin();
                    const char* buf_end = state->LogsBuffer.end();

                    ImGuiListClipper clipper;
                    clipper.Begin( state->LogsOffset.Size );
                    while (clipper.Step()) {
                        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
                            const char* line_start = buf + state->LogsOffset[line_no];
                            const char* line_end   = ( line_no + 1 < state->LogsOffset.Size )
                                                         ? ( buf + state->LogsOffset[line_no + 1] - 1 )
                                                         : buf_end;
                            ImGui::TextUnformatted( line_start, line_end );
                        }
                    }

                    if (state->LogsAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                        ImGui::SetScrollHereY( 1.0f );
                    }

                    ImGui::EndChild();

                    ImGui::End();
                }
            }

            if (state->ShowStatsWindow) {
                if (ImGui::Begin( "Stats", &state->ShowStatsWindow )) {

                    ImGui::SeparatorText( "RTTI" );
                    ImGui::Text( "Hits: %d", rtti::TypeRegistry::get_rtti_hits() );

                    ImGui::SeparatorText( "Rendering" );
                    const auto& stats = vid->Gfx->get_stats();
                    ImGui::Text( "GPU Duration: %.3f ms", stats.gpu_duration_ms );
                    ImGui::Text( "Input Assembler Primitives: %llu", stats.input_primitives );
                    ImGui::Text( "Input Assembler Vertices: %llu", stats.input_vertices );
                    ImGui::Text( "Vertex Shader Invocations: %llu", stats.vs_invocations );
                    ImGui::Text( "Pixel Shader Invocations: %llu", stats.ps_invocations );
                    ImGui::Text( "Clipping Primitives: %llu", stats.clipping_primitives );
                    ImGui::Text( "Clipping Invocations: %llu", stats.clipping_invocations );

                    ImGui::SeparatorText( "ECS Debug" );
                    ImGui::Text(
                        "Entity count:\n Total: %zu\n Alive: %zu", it.world().get_entity_count(),
                        it.world().get_entity_count( true )
                    );

                    if (ImGui::Button( "Spawn Window" )) {
                        it.world()
                            .entity()
                            .add<WindowComponent>( WindowComponent{ .Resolution = Vec2i( 300, 300 ), .Visible = true } )
                            .build();
                    }
                }
                ImGui::End();
            }
        } );

    // TODO: refactor this to use Timers
    scene.get_ecs()
        .system( "EngineEditorPlugin_TitleBarUpdater" )
        .with<WindowComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .each( []( EcsIterState& it ) {
            // auto vid  = it.world().get_singleton<VideoServices>();
            // auto time = it.world().get_singleton<TimeComponent>();

            // auto window = it.get_component<WindowComponent>();
            // if (time->accumulator >= 0.5) {
            //     update_window_title( vid->window, it.entity(), window, time->fps, it.delta_time() );
            // }
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_InputUI" )
        .with<IOServices>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( EcsIterState& it ) {
            auto state = it.world().get_singleton<EditorState>();
            if (!state->ShowInputsWindow)
                return;

            auto io = it.world().get_singleton<IOServices>();

            if (ImGui::Begin( "Input Debug", &state->ShowInputsWindow )) {
                {
                    ImGui::SeparatorText( "Actions" );
                    DynamicArray<StringView> actions;
                    io->Inputs->action_list( actions );

                    if (ImGui::BeginTable( "ActionsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg )) {
                        ImGui::TableSetupColumn( "Name" );
                        ImGui::TableSetupColumn( "Held" );
                        ImGui::TableSetupColumn( "Pressed" );
                        ImGui::TableSetupColumn( "Released" );
                        ImGui::TableHeadersRow();

                        for (const auto& action : actions) {
                            bool held     = io->Inputs->action_is_held( action.data() );
                            bool pressed  = io->Inputs->action_is_pressed( action.data() );
                            bool released = io->Inputs->action_is_released( action.data() );

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
                        "Pos", io->Inputs->get_mouse_position().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                    ImGui::DragFloat2(
                        "Delta", io->Inputs->get_mouse_delta().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                    ImGui::DragFloat2(
                        "Wheel", io->Inputs->get_mouse_wheel().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                }
            }
            ImGui::End();
        } );

    scene.get_ecs()
        .observer( "EngineEditorPlugin_Cleanup" )
        .on<EditorState>( EcsCoreEvent::OnRemove )
        .run( []( EcsIterState& it ) {
            auto state = it.world().get_singleton<EditorState>();
            auto vid   = it.world().get_singleton<VideoServices>();
            if (state && vid) {
                if (state->ViewportRT)
                    vid->Gfx->destroy_rid( state->ViewportRT );
                if (state->ViewportDT)
                    vid->Gfx->destroy_rid( state->ViewportDT );
                if (state->GameViewRT)
                    vid->Gfx->destroy_rid( state->GameViewRT );
                if (state->GameViewDT)
                    vid->Gfx->destroy_rid( state->GameViewDT );
            }
        } );
}

void unregister_editor_plugin( Scene& scene )
{
    scene.get_ecs().remove_singleton<EditorState>();
    unregister_gui_plugin( scene );
}

} // namespace nc::editor
