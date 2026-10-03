// clang-format off
#include <imgui_internal.h>
#include <ImGuizmo.h> // must come after imgui includes
// clang-format on

#include <filesystem>

#include <editor/ncore_editor.h>
#include <ncore/core/rid.h>
#include <ncore/game_world.h>
#include <ncore/runtime/components/material.h>
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

static bool is_internal_component( const rtti::TypeInfo* type )
{
    return type == rtti::TypeRegistry::find<NodeRefComponent>();
}

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

namespace {

using namespace nc::rtti;

bool draw_record_fields( const RecordInfo* record, void* data, int depth );

void begin_labeled_row( StringView label )
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted( label.data(), label.data() + label.size() );
    ImGui::SameLine( ImGui::GetWindowContentRegionMax().x * 0.4f );
    ImGui::SetNextItemWidth( -FLT_MIN );
}

ImGuiDataType imgui_data_type( TypeKind kind )
{
    switch (kind) {
        case TypeKind::INT8:
            return ImGuiDataType_S8;
        case TypeKind::UINT8:
            return ImGuiDataType_U8;
        case TypeKind::INT16:
            return ImGuiDataType_S16;
        case TypeKind::UINT16:
            return ImGuiDataType_U16;
        case TypeKind::INT32:
            return ImGuiDataType_S32;
        case TypeKind::UINT32:
            return ImGuiDataType_U32;
        case TypeKind::INT64:
            return ImGuiDataType_S64;
        case TypeKind::UINT64:
            return ImGuiDataType_U64;
        case TypeKind::FLOAT:
            return ImGuiDataType_Float;
        case TypeKind::DOUBLE:
            return ImGuiDataType_Double;
        default:
            return ImGuiDataType_COUNT;
    }
}

static int string_resize_callback( ImGuiInputTextCallbackData* cb )
{
    if (cb->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* s = static_cast<String*>( cb->UserData );
        s->resize( static_cast<size_t>( cb->BufTextLen ) );
        cb->Buf = s->data();
    }
    return 0;
}

bool draw_leaf_value( const TypeInfo* type, void* ptr )
{
    switch (type->kind) {
        case TypeKind::BOOL:
            return ImGui::Checkbox( "##v", static_cast<bool*>( ptr ) );

        case TypeKind::INT8:
        case TypeKind::UINT8:
        case TypeKind::INT16:
        case TypeKind::UINT16:
        case TypeKind::INT32:
        case TypeKind::UINT32:
        case TypeKind::INT64:
        case TypeKind::UINT64: {
            return ImGui::DragScalar( "##v", imgui_data_type( type->kind ), ptr, 1.0f );
        }

        case TypeKind::FLOAT:
        case TypeKind::DOUBLE: {
            return ImGui::DragScalar( "##v", imgui_data_type( type->kind ), ptr, 0.01f );
        }

        case TypeKind::ENUM: {
            auto* info       = static_cast<const EnumInfo*>( type );
            int64_t current  = info->get_value( ptr );
            StringView cname = info->get_name( current );
            bool changed     = false;

            if (ImGui::BeginCombo( "##v", cname.data() )) {
                for (const auto& elem : info->elements()) {
                    bool selected = elem.value == current;
                    if (ImGui::Selectable( elem.name.data(), selected )) {
                        info->set_value( ptr, elem.value );
                        changed = true;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            return changed;
        }

        case TypeKind::STRING: {
            auto* str = static_cast<String*>( ptr );
            return ImGui::InputText(
                "##v", str->data(), str->capacity() + 1, ImGuiInputTextFlags_CallbackResize, string_resize_callback, str
            );
        }

        default: {
            // Vectors, pointers and anything unknown: read-only textual dump.
            String out;
            type->to_string( out, ptr );
            ImGui::TextDisabled( "%s", out.c_str() );
            return false;
        }
    }
}

bool draw_labeled_value( StringView label, const TypeInfo* type, void* ptr, int depth )
{
    if (!type) {
        begin_labeled_row( label );
        ImGui::TextDisabled( "<unregistered type>" );
        return false;
    }

    if (type->kind == TypeKind::RECORD) {
        bool changed = false;
        String lbl( label );
        if (depth < 8 && ImGui::TreeNodeEx( lbl.c_str(), ImGuiTreeNodeFlags_DefaultOpen )) {
            changed = draw_record_fields( static_cast<const RecordInfo*>( type ), ptr, depth + 1 );
            ImGui::TreePop();
        }
        return changed;
    }

    begin_labeled_row( label );
    return draw_leaf_value( type, ptr );
}

bool draw_field( const FieldInfo& field, void* data, int depth )
{
    if (field.is( PropertyFlags::HIDDEN )) {
        return false;
    }

    const TypeInfo* type = field.get_type();
    StringView name{ field.name.data(), field.name.size() };
    void* ptr = field.get_void_ptr( data );

    bool changed = false;

    ImGui::PushID( name.data(), name.data() + name.size() );
    ImGui::BeginDisabled( field.is( PropertyFlags::READ_ONLY ) );

    if (field.qualifier.is_pointer()) {
        // Pointers are shown, never edited.
        begin_labeled_row( name );
        void* target = field.get_as<void*>( data );
        if (field.qualifier.is_cstring) {
            const char* s = static_cast<const char*>( target );
            ImGui::TextDisabled( "%s", s ? s : "(null)" );
        } else {
            ImGui::TextDisabled( "%p", target );
        }
    } else if (field.qualifier.is_array()) {
        std::string lbl = std::string( name ) + " [" + std::to_string( field.qualifier.array_length ) + "]";
        if (type && ImGui::TreeNodeEx( lbl.c_str(), ImGuiTreeNodeFlags_None )) {
            auto* base = static_cast<uint8_t*>( ptr );
            for (uint32_t i = 0; i < field.qualifier.array_length; ++i) {
                ImGui::PushID( static_cast<int>( i ) );
                std::string idx = "[" + std::to_string( i ) + "]";
                changed |= draw_labeled_value( idx, type, base + i * type->size, depth + 1 );
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    } else {
        changed = draw_labeled_value( name, type, ptr, depth );
    }

    ImGui::EndDisabled();
    ImGui::PopID();
    return changed;
}

bool draw_record_fields( const RecordInfo* record, void* data, int depth )
{
    bool changed = false;
    for (const auto& field : record->fields()) {
        changed |= draw_field( field, data, depth );
    }
    return changed;
}

} // namespace

static bool draw_component_properties( const rtti::TypeInfo* type, void* data )
{
    NC_ASSERT( type->is_record() );

    auto record = static_cast<const rtti::RecordInfo*>( type );
    if (record->field_count() == 0) {
        return false;
    }

    return draw_record_fields( record, data, 0 );
}

void NCAPI_EDITOR register_editor_plugin( Scene& scene )
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
        editor_state->LogsBuffer.append( msg.payload.c_str(), msg.payload.c_str() + msg.payload.size() );
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
        .order( -900 )
        .run( []( EcsIterState& it ) {
            ( void ) it;

            ImGuizmo::SetOrthographic( false );
            ImGuizmo::BeginFrame();

            // auto view_matrix = vid->Renderer->world_get_view_matrix().data();
            // auto proj_matrix = vid->Renderer->world_camera_get_projection().data();

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

            RenderService::RenderFrameDesc desc;
            desc.camera       = state->EditorCamSource;
            desc.draw_spatial = false;
            desc.draw_canvas  = true;
            desc.to_screen    = true;
            vid->Renderer->render_frame( desc );
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_ConfigureDocking" )
        .with<GuiStateComponent>()
        .with<EditorState>()
        .in( EcsSystemPhase::PRE_UPDATE )
        // Must run before every other PRE_UPDATE system that calls ImGui::Begin():
        // ImGuizmo::BeginFrame() (order -900) opens a "gizmo" window, and ImGui
        // undocks the whole tree if the dockspace is submitted after any window.
        .order( -950 )
        .run( []( EcsIterState& it ) {
            auto state = it.get_component<EditorState>();

            ImGuiID dockspace_id = ImGui::DockSpaceOverViewport( 0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode );
            state->DockspaceId   = dockspace_id;

            // DockSpaceOverViewport() creates the dockspace node before this runs, so
            // "node is null" never happens. The real first-run signal is that ImGui has
            // no saved layout to load. Latch it on frame 1: the .ini is written a few
            // seconds after the first modification, so existence is stable for that frame.
            static bool s_latched = false;
            static bool s_pending = false;
            static bool s_built   = false;
            if (!s_latched) {
                s_latched       = true;
                const char* ini = ImGui::GetIO().IniFilename;
                s_pending       = ( !ini ) || ( !std::filesystem::exists( ini ) );
                NC_LOG_INFO_C(
                    log::GUI, "ConfigureDocking: ini='{}' build_default_layout={}", ini ? ini : "<null>",
                    s_pending ? 1 : 0
                );
            }

            // The swapchain still reports 1x1 on the first frames, and a layout built
            // against WorkSize=1x1 bakes ~1px dock nodes (invisible panels) forever.
            // Wait for a real viewport before splitting.
            bool build = false;
            if (s_pending && !s_built) {
                const ImVec2 ws = ImGui::GetMainViewport()->WorkSize;
                if (ws.x >= 64.0f && ws.y >= 64.0f) {
                    build     = true;
                    s_built   = true;
                    s_pending = false;
                }
            }

            if (build) {
                ImGui::DockBuilderRemoveNode( dockspace_id );
                ImGui::DockBuilderAddNode( dockspace_id, ImGuiDockNodeFlags_DockSpace );
                ImGui::DockBuilderSetNodeSize( dockspace_id, ImGui::GetMainViewport()->WorkSize );
                ImGuiID dock_main = dockspace_id;
                ImGuiID dock_top  = ImGui::DockBuilderSplitNode( dock_main, ImGuiDir_Up, 0.05f, nullptr, &dock_main );
                ImGuiID dock_left = ImGui::DockBuilderSplitNode( dock_main, ImGuiDir_Left, 0.22f, nullptr, &dock_main );
                ImGuiID dock_right =
                    ImGui::DockBuilderSplitNode( dock_main, ImGuiDir_Right, 0.22f, nullptr, &dock_main );
                ImGuiID dock_game = ImGui::DockBuilderSplitNode( dock_main, ImGuiDir_Down, 0.55f, nullptr, &dock_main );
                ImGuiID dock_logs = ImGui::DockBuilderSplitNode( dock_game, ImGuiDir_Down, 0.45f, nullptr, &dock_game );
                ImGui::DockBuilderDockWindow( "Toolbar", dock_top );
                ImGui::DockBuilderDockWindow( "Scene Tree", dock_left );
                ImGui::DockBuilderDockWindow( "Scene View", dock_main );
                ImGui::DockBuilderDockWindow( "Game View", dock_game );
                ImGui::DockBuilderDockWindow( "Logs", dock_logs );
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
                    vid->Renderer->destroy_rid( state->ViewportRT );
                if (state->ViewportDT)
                    vid->Renderer->destroy_rid( state->ViewportDT );

                Vec2i vp_size( static_cast<int>( img_size.x ), static_cast<int>( img_size.y ) );
                state->ViewportRT =
                    vid->Renderer->texture_render_create( vp_size, gfx::TextureFormat::RGBA8_UNORM_SRGB );
                state->ViewportDT   = vid->Renderer->texture_render_create( vp_size, gfx::TextureFormat::D32_FLOAT );
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

                auto& cam_attribs   = vid->Renderer->camera_get_attribs( state->EditorCamSource );
                auto projection_mat = vid->Renderer->camera_get_perspective( state->EditorCamSource );
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

                const auto& stats = vid->Renderer->get_stats();
                ImGui::Text( "GPU time: %.3f ms", stats.gpu_duration_ms );
                for (uint32_t i = 0; i < stats.pass_count; ++i)
                    ImGui::Text( "  pass[%u]: %.3f ms", i, stats.pass_duration_ms[i] );
                ImGui::Text( "  compute: %.3f ms", stats.compute_duration_ms );
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
            if (!state->ShowGameView) {
                state->RenderGameView = false;
                return;
            }

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
            state->RenderGameView = ImGui::Begin(
                "Game View", &state->ShowGameView, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
            );
            if (state->RenderGameView) {
                if (state->GameViewRT) {
                    ImVec2 img_size = ImGui::GetContentRegionAvail();
                    ImTextureID tex_id =
                        reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( state->GameViewRT.value ) );
                    ImGui::Image( tex_id, img_size );
                }
            }
            ImGui::End();
            ImGui::PopStyleVar( 2 );
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_Panels" )
        .with<EditorState>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( EcsIterState& it ) {
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
                        if (ImGui::MenuItem( "Game View" )) {
                            state->ShowGameView = true;
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
                        for (auto mat_entity : q.entities()) {
                            auto mat      = mat_entity.get_component<MaterialComponent>();
                            mat->DrawMode = state->DrawWireframe ? gfx::FillMode::WIREFRAME : gfx::FillMode::SOLID;
                            mat_entity.mark_component_modified<MaterialComponent>();
                        }
                    }
                }
                ImGui::End();
            }

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
                                if (is_internal_component( type ))
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
                            if (!type || is_internal_component( type ))
                                continue;

                            void* data_ptr = state->SelectedNode->get_component( type );
                            if (!data_ptr)
                                continue;

                            ImGui::Separator();
                            if (ImGui::CollapsingHeader( type->name, ImGuiTreeNodeFlags_DefaultOpen )) {
                                ImGui::PushID( static_cast<int>( id << 16 | type->id.value ) );
                                if (draw_component_properties( type, data_ptr )) {
                                    state->SelectedNode->mark_component_modified( type );
                                }

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
                }
                ImGui::End();
            }

            if (state->ShowStatsWindow) {
                if (ImGui::Begin( "Stats", &state->ShowStatsWindow )) {

                    ImGui::SeparatorText( "RTTI" );
                    ImGui::Text( "Hits: %d", rtti::TypeRegistry::get_rtti_hits() );

                    ImGui::SeparatorText( "Rendering" );
                    const auto& stats = vid->Renderer->get_stats();
                    ImGui::Text( "GPU Duration: %.3f ms", stats.gpu_duration_ms );
                    for (uint32_t i = 0; i < stats.pass_count; ++i)
                        ImGui::Text( "  pass[%u]: %.3f ms", i, stats.pass_duration_ms[i] );
                    ImGui::Text( "  compute: %.3f ms", stats.compute_duration_ms );
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
                        WindowComponent spawn{};
                        spawn.Resolution = Vec2i( 300, 300 );
                        spawn.Visible    = true;
                        it.world().entity().add<WindowComponent>( spawn ).build();
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
                    auto actions = io->Inputs->action_list();

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
                    vid->Renderer->destroy_rid( state->ViewportRT );
                if (state->ViewportDT)
                    vid->Renderer->destroy_rid( state->ViewportDT );
                if (state->GameViewRT)
                    vid->Renderer->destroy_rid( state->GameViewRT );
                if (state->GameViewDT)
                    vid->Renderer->destroy_rid( state->GameViewDT );
            }
        } );
}

void NCAPI_EDITOR unregister_editor_plugin( Scene& scene )
{
    scene.get_ecs().remove_singleton<EditorState>();
    unregister_gui_plugin( scene );
}

} // namespace nc::editor
