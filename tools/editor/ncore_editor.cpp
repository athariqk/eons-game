#include "ncore_editor.h"

#include <imgui_internal.h>

#include <ncore/core/color.h>
#include <ncore/core/quaternion.h>
#include <ncore/core/rid.h>
#include <ncore/core/types.h>
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
    Scene* current_scene     = nullptr;
    ImGuiID dockspace_id     = 0;
    bool stats_window        = false;
    bool inputs_window       = false;
    Node* selected_node      = nullptr;
    char rename_buffer[256]  = {};
    Node* renaming_node      = nullptr;
    bool open_rename         = false;
    char add_comp_filter[64] = {};
    NSTRUCT(
        EngineEditorState, NC_F( EngineEditorState, current_scene ) NC_F( EngineEditorState, dockspace_id )
                               NC_F( EngineEditorState, stats_window ) NC_F( EngineEditorState, inputs_window )
                                   NC_F( EngineEditorState, selected_node )
    )
};

static void draw_scene_tree_node( Node& node, EngineEditorState& state )
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull;
    if (node.get_child_count() == 0) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (node == state.selected_node) {
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
            auto nlen = std::min( n.size(), sizeof( state.rename_buffer ) - 1 );
            std::memcpy( state.rename_buffer, n.data(), nlen );
            state.rename_buffer[nlen] = '\0';
            state.renaming_node       = &node;
            state.open_rename         = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button( "Delete" )) {
            node.destroy();
            state.selected_node = nullptr; // avoids crashing the ECS
            ImGui::CloseCurrentPopup();
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

static void draw_vec2_widget( const char* label, void* ptr, bool editable )
{
    if (editable)
        ImGui::DragFloat2( label, static_cast<Vec2*>( ptr )->data(), 0.01f );
    else {
        auto* v = static_cast<Vec2*>( ptr );
        ImGui::Text( "%s: (%.2f, %.2f)", label, static_cast<double>( v->x ), static_cast<double>( v->y ) );
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
        { rtti::TypeRegistry::find<Vec2>()->id, draw_vec2_widget },
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

static void draw_field_widget( void* instance, const rtti::FieldInfo& field )
{
    auto* type = field.get_type();
    if (!type)
        return;

    void* ptr = field.get_void_ptr( instance );
    if (!ptr)
        return;

    bool is_editable  = field.is( rtti::PropertyFlags::EDITABLE ) && !field.is( rtti::PropertyFlags::READ_ONLY );
    const char* label = field.name.data();

    auto* draw = find_widget( field.type_id );
    if (draw) {
        ( *draw )( label, ptr, is_editable );
        return;
    }

    if (type->category == rtti::FieldCategory::STRING) {
        ImGui::Text( "%s: \"%s\"", label, static_cast<const char*>( ptr ) );
    } else if (type->is_record()) {
        auto* record = static_cast<const rtti::RecordInfo*>( type );
        if (ImGui::TreeNode( label )) {
            for (auto& sub_field : record->fields()) {
                ImGui::PushID( sub_field.name.data() );
                draw_field_widget( ptr, sub_field );
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    } else {
        ImGui::Text( "%s: <?>", label );
    }
}

void register_editor_plugin( Scene& scene )
{
    auto editor_state           = scene.get_ecs().emplace_singleton<EngineEditorState>();
    editor_state->current_scene = &scene;

    scene.get_ecs()
        .system( "EngineEditorFeature_Init" )
        .with<EngineEditorState>()
        .with<GuiStateComponent>()
        .in( EcsSystemPhase::INIT )
        .order( 20 )
        .run( []( QueryContext& ctx ) {
            auto gui_state = ctx.get_component<GuiStateComponent>();

            if (gui_state->imctx) {
                ImGui::SetCurrentContext( gui_state->imctx );
            }

            StyleColorsEditor();
            StyleSizesEditor();

            // TODO: make this into a prefab
            ctx.world()
                .entity( "EditorFlyCam" )
                .with<Transform3DComponent>( { Vec3(), Quaternion::identity(), Vec3( 1, 1, 1 ) } )
                .with<CameraComponent>()
                .with<InputComponent>()
                .build();
        } );

    scene.get_ecs()
        .observer( "EngineEditorFeature_SwapChainResizedDebug" )
        .with<SwapChainComponent>()
        .event<SwapChainResizedComponent>()
        .each( []( QueryContext& ctx, EcsEntity ) {
            auto resized = ctx.event_payload<SwapChainResizedComponent>();
            String stringified;
            rtti::TypeRegistry::to_string<Vec2>( stringified, &resized->size );
            NC_LOG_DEBUG_C( log::GRAPHICS, "SwapChainResized: size={}", stringified );
        } );

    scene.get_ecs()
        .system( "EngineEditorFeature_ConfigureDocking" )
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
        .system( "EngineEditorFeature_Panels" )
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
                auto root = state->current_scene->root();

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

                if (state->open_rename) {
                    ImGui::OpenPopup( "Entity Rename" );
                    state->open_rename = false;
                }

                if (ImGui::BeginPopupModal( "Entity Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize )) {
                    ImGui::InputText(
                        "##rename", state->rename_buffer, sizeof( state->rename_buffer ),
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll
                    );
                    bool confirmed = ImGui::Button( "OK" ) || ImGui::IsKeyPressed( ImGuiKey_Enter );
                    if (confirmed && state->renaming_node && state->rename_buffer[0]) {
                        state->renaming_node->set_name( state->rename_buffer );
                        state->renaming_node = nullptr;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button( "Cancel" )) {
                        state->renaming_node = nullptr;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }

            {
                if (ImGui::Begin( "Inspector" )) {
                    if (state->selected_node) {
                        auto name   = state->selected_node->get_name();
                        auto id     = state->selected_node->get_id();
                        auto active = state->selected_node->get_active();

                        ImGui::Text( "Node: %s", name.data() );
                        ImGui::Text( "ID: %llu", id );
                        ImGui::Checkbox( "Active", active );

                        auto& ecs = state->current_scene->get_ecs();
                        for (auto& comp : state->selected_node->get_components()) {
                            auto* type = ecs.resolve_component( comp.EcsId );
                            if (!type)
                                continue;
                            if (type == rtti::TypeRegistry::find<NodeRefComponent>())
                                continue;

                            void* comp_data = state->selected_node->get_component( type );
                            if (!comp_data)
                                continue;

                            ImGui::Separator();
                            if (ImGui::CollapsingHeader( type->name, ImGuiTreeNodeFlags_DefaultOpen )) {
                                if (type->is_record()) {
                                    auto* record = static_cast<const rtti::RecordInfo*>( type );
                                    for (auto& field : record->fields()) {
                                        ImGui::PushID( field.name.data() );
                                        draw_field_widget( comp_data, field );
                                        ImGui::PopID();
                                    }
                                }

                                ImGui::PushID( static_cast<int>( id << 32 | type->id.value ) );
                                if (ImGui::SmallButton( "Remove" )) {
                                    state->selected_node->remove_component( type );
                                }
                                ImGui::BeginDisabled( !comp.CanToggleActive );
                                ImGui::Checkbox( "Active", &comp.Active );
                                ImGui::EndDisabled();
                                ImGui::PopID();
                            }
                        }

                        ImGui::Separator();

                        if (ImGui::Button( "+ Add Component", ImVec2( -FLT_MIN, 0 ) ))
                            ImGui::OpenPopup( "AddComponent" );

                        if (ImGui::BeginPopup( "AddComponent" )) {
                            ImGui::InputTextWithHint(
                                "##filter", "Search...", state->add_comp_filter, sizeof( state->add_comp_filter )
                            );

                            ImGui::BeginChild( "##complist", ImVec2( 0, 200 ), ImGuiChildFlags_Borders );
                            for (auto type : ecs.get_component_types()) {
                                if (!type->is_record())
                                    continue;
                                if (type == rtti::TypeRegistry::find<NodeRefComponent>())
                                    continue;
                                if (state->add_comp_filter[0] && !strstr( type->name, state->add_comp_filter ))
                                    continue;

                                bool already = state->selected_node->has_component( type );
                                ImGui::BeginDisabled( already );
                                if (ImGui::Selectable( type->name )) {
                                    state->selected_node->emplace_component( type );
                                    state->add_comp_filter[0] = '\0';
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::EndDisabled();
                            }
                            ImGui::EndChild();
                            ImGui::EndPopup();
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

#if !defined( NC_DIST )
    scene.get_ecs()
        .system( "EngineEditorFeature_HotReload" )
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.world().get_singleton<IoServices>();

            if (ImGui::IsKeyPressed( ImGuiKey_F5 )) {
                NC_LOG_INFO_C( log::GRAPHICS, "Hot-reloading" );
                io->resources->load<MaterialTemplate>( "shaders/pbr.slang", true );
                io->resources->load<MaterialTemplate>( "materials/world_instance.material", true );
                io->resources->load<MaterialTemplate>( "shaders/canvas.slang", true );
                io->resources->load<MaterialTemplate>( "materials/canvas.material", true );
            }
        } );
#endif

    // TODO: refactor this to use Timers
    scene.get_ecs()
        .system( "EngineEditorFeature_TitleBarUpdater" )
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
        .system( "EngineEditorFeature_InputUI" )
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
            }
            ImGui::End();
        } );
}

} // namespace nc::editor
