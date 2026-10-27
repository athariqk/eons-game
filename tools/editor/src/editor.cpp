#include "editor.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <imgui_internal.h>
#include <mutex>
#include <utility>

#include <ncore/application.h>
#include <ncore/runtime/components/camera.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/time.h>
#include <ncore/runtime/ecs/ecs_query.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/log.h>

#include "plugins/gui_plugin.h"

#include <io/asset_importer.h>
#include <io/editor_assets.h>

namespace nc::editor {

namespace {

void clear_staged_logs();
log::ListenerToken install_log_listener();

} // namespace

void stage( const log::LogMsg& msg );

namespace {

constexpr const char* DEFAULT_SETTINGS_PATH = "editor.ini";

bool slot_less( const Ptr<EditorPanel>& a, const Ptr<EditorPanel>& b )
{
    if (a->get_slot() != b->get_slot())
        return static_cast<int>( a->get_slot() ) < static_cast<int>( b->get_slot() );
    return a->get_order() < b->get_order();
}

} // namespace

//------------------------------------------------------------------------------
// EditorInterface
//------------------------------------------------------------------------------

EditorPanel* EditorInterface::add_panel( DockSlot slot, int order, Ptr<EditorPanel> panel )
{
    return editor.register_panel( slot, order, std::move( panel ) );
}

void EditorInterface::add_command( StringView menu, StringView label, std::function<void()> on_invoke )
{
    editor.register_command( menu, label, std::move( on_invoke ) );
}

void EditorInterface::add_inspector_handler( Ptr<InspectorHandler> handler )
{
    editor.register_inspector_handler( std::move( handler ) );
}

void EditorInterface::add_property_widget( const rtti::TypeInfo* type, PropertyWidgetFn widget )
{
    editor.register_property_widget( type, std::move( widget ) );
}

EditorContext& EditorInterface::get_ctx() const
{
    return editor.get_ctx();
}

Scene& EditorInterface::get_scene() const
{
    return editor.get_scene();
}

Editor& EditorInterface::get_editor() const
{
    return editor;
}

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

void EditorSettings::load( const char* path )
{
    std::ifstream in( path );
    if (!in)
        return;

    std::string key;
    while (std::getline( in, key )) {
        const auto eq = key.find( '=' );
        if (eq == std::string::npos)
            continue;

        const std::string name  = key.substr( 0, eq );
        const std::string value = key.substr( eq + 1 );

        if (name == "gizmo.world")
            global_xform_gizmo = value == "1";
        else if (name == "gizmo.operation")
            xform_gizmo_operation = std::atoi( value.c_str() );
        else if (name == "render.wireframe")
            draw_wireframe = value == "1";
        else if (name.rfind( "panel.", 0 ) == 0)
            panel_visible[String( name.c_str() + 6 )] = value == "1";
    }
}

void EditorSettings::save( const char* path ) const
{
    std::ofstream out( path, std::ios::trunc );
    if (!out)
        return;

    out << "gizmo.world=" << ( global_xform_gizmo ? 1 : 0 ) << '\n';
    out << "gizmo.operation=" << xform_gizmo_operation << '\n';
    out << "render.wireframe=" << ( draw_wireframe ? 1 : 0 ) << '\n';
    for (const auto& [title, visible] : panel_visible)
        out << "panel." << title.c_str() << '=' << ( visible ? 1 : 0 ) << '\n';
}

//------------------------------------------------------------------------------
// Editor
//------------------------------------------------------------------------------

void register_editor_component_types( Scene& scene )
{
    auto& world = scene.get_ecs();

    auto reg = [&world]( const rtti::TypeInfo* type ) {
        if (type)
            world.register_component_type( type );
    };

    reg( rtti::TypeRegistry::find<EditorCameraTag>() );
    reg( rtti::TypeRegistry::find<SelectedTag>() );
    reg( rtti::TypeRegistry::find<GuiStateComponent>() );
}

Editor::Editor( Scene& p_scene ) : scene( p_scene )
{
    ctx.scene  = &p_scene;
    ctx.loader = &p_scene.get_resource_loader();

    assets_index = std::make_unique<AssetIndex>();
    importer     = std::make_unique<AssetImporter>();

    if (auto* assets_service = p_scene.get_app_ctx()->services.resolve<AssetService>())
        importer->init( *p_scene.get_app_ctx(), p_scene.get_resource_loader(), *assets_service );

    settings.load( DEFAULT_SETTINGS_PATH );
    ctx.global_xform_gizmo    = settings.global_xform_gizmo;
    ctx.xform_gizmo_operation = settings.xform_gizmo_operation;
    ctx.draw_wireframe        = settings.draw_wireframe;

    log_token = install_log_listener();
}

Editor::~Editor()
{
    // unwind plugins in reverse registration order, then let the panels go.
    EditorInterface iface( *this );
    for (auto it = plugins.rbegin(); it != plugins.rend(); ++it) {
        active_plugin = static_cast<int>( std::distance( plugins.begin(), it.base() ) ) - 1;
        ( *it )->exit( iface );
    }
    active_plugin = -1;

    plugins.clear();
    commands.clear();
    inspector_handlers.clear();
    property_widgets.clear();
    panels.clear();

    log_token = {};
    clear_staged_logs();
}

void Editor::add_plugin( Ptr<EditorPlugin> plugin )
{
    if (!plugin)
        return;

    active_plugin = static_cast<int>( plugins.size() );
    EditorInterface iface( *this );
    plugin->enter( iface );

    plugins.push_back( std::move( plugin ) );
    active_plugin = -1;

    sort_panels();
    apply_settings();
}

EditorPanel* Editor::register_panel( DockSlot slot, int order, Ptr<EditorPanel> panel )
{
    // The registration call is authoritative: a panel is docked where the
    // plugin asked, not where its constructor happened to default.
    panel->set_slot( slot );
    panel->set_order( order );
    panel->set_owner( active_plugin );

    panels.push_back( std::move( panel ) );
    return panels.back().get();
}

void Editor::register_command( StringView menu, StringView label, std::function<void()> on_invoke )
{
    EditorCommand command;
    command.menu   = String( menu.data(), menu.size() );
    command.label  = String( label.data(), label.size() );
    command.invoke = std::move( on_invoke );
    command.owner  = active_plugin;
    commands.push_back( std::move( command ) );
}

void Editor::register_inspector_handler( Ptr<InspectorHandler> handler )
{
    if (handler)
        inspector_handlers.push_back( std::move( handler ) );
}

void Editor::register_property_widget( const rtti::TypeInfo* type, PropertyWidgetFn widget )
{
    if (!type || !widget)
        return;

    property_widgets.push_back( PropertyWidget{ type, std::move( widget ), active_plugin } );
}

void Editor::sort_panels()
{
    std::stable_sort( panels.begin(), panels.end(), slot_less );
}

void Editor::apply_settings()
{
    for (auto& panel : panels) {
        const auto it = settings.panel_visible.find( panel->get_title() );
        if (it != settings.panel_visible.end())
            panel->get_visible() = it->second;

        settings.panel_visible[panel->get_title()] = panel->get_visible();
    }
}

void Editor::begin_frame()
{
    auto& world = scene.get_ecs();

    ctx.scene              = &scene;
    ctx.ecs_world          = &world;
    ctx.loader             = &scene.get_resource_loader();
    ctx.assets             = assets_index.get();
    ctx.importer           = importer.get();
    ctx.commands           = &commands;
    ctx.inspector_handlers = &inspector_handlers;
    ctx.property_widgets   = &property_widgets;

    if (auto* vid = world.get_singleton<VideoServices>())
        ctx.renderer = vid->renderer;
    if (auto* io = world.get_singleton<IOServices>())
        ctx.inputs = io->inputs;

    ctx.time = world.get_singleton<TimeComponent>();

    selection_state.refresh( world );
    ctx.selection_state      = &selection_state;
    ctx.log_store            = &log_store;
    ctx.selected_eid         = selection_state.get_selected_eid();
    ctx.any_viewport_hovered = false;
    ctx.any_viewport_focused = false;

    // the staging buffer must stay bounded even while the console is closed.
    drain_logs();
}

void Editor::draw_dockspace()
{
    const ImGuiID dockspace_id = ImGui::DockSpaceOverViewport( 0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode );

    // The first-run signal is that ImGui has no saved layout to load. Latch it
    // once: the .ini is written a few seconds after the first modification.
    if (!is_layout_checked) {
        is_layout_checked = true;
        const char* ini   = ImGui::GetIO().IniFilename;
        is_layout_pending = ( !ini ) || ( !std::filesystem::exists( ini ) );
    }

    // The swapchain still reports 1x1 on the first frames, and a layout built
    // against WorkSize=1x1 bakes ~1px dock nodes (invisible panels) forever.
    if (!is_layout_pending || is_layout_built)
        return;

    const ImVec2 ws = ImGui::GetMainViewport()->WorkSize;
    if (ws.x < 64.0f || ws.y < 64.0f)
        return;

    is_layout_built   = true;
    is_layout_pending = false;
    build_default_layout( dockspace_id );
}

void Editor::build_default_layout( unsigned int p_dockspace_id )
{
    ImGui::DockBuilderRemoveNode( p_dockspace_id );
    ImGui::DockBuilderAddNode( p_dockspace_id, ImGuiDockNodeFlags_DockSpace );
    ImGui::DockBuilderSetNodeSize( p_dockspace_id, ImGui::GetMainViewport()->WorkSize );

    ImGuiID center = p_dockspace_id;
    ImGuiID top    = ImGui::DockBuilderSplitNode( center, ImGuiDir_Up, 0.05f, nullptr, &center );
    ImGuiID left   = ImGui::DockBuilderSplitNode( center, ImGuiDir_Left, 0.22f, nullptr, &center );
    ImGuiID right  = ImGui::DockBuilderSplitNode( center, ImGuiDir_Right, 0.22f, nullptr, &center );
    ImGuiID bottom = ImGui::DockBuilderSplitNode( center, ImGuiDir_Down, 0.28f, nullptr, &center );

    ImGuiID left_lower  = ImGui::DockBuilderSplitNode( left, ImGuiDir_Down, 0.55f, nullptr, &left );
    ImGuiID right_lower = ImGui::DockBuilderSplitNode( right, ImGuiDir_Down, 0.55f, nullptr, &right );

    for (const auto& panel : panels) {
        ImGuiID target = p_dockspace_id;
        switch (panel->get_slot()) {
            case DockSlot::Top:
                target = top;
                break;
            case DockSlot::LeftUpper:
                target = left;
                break;
            case DockSlot::LeftLower:
                target = left_lower;
                break;
            case DockSlot::RightUpper:
                target = right;
                break;
            case DockSlot::RightLower:
                target = right_lower;
                break;
            case DockSlot::Bottom:
                target = bottom;
                break;
            case DockSlot::Center:
                target = center;
                break;
            case DockSlot::Floating:
                continue;
        }
        ImGui::DockBuilderDockWindow( panel->get_title().c_str(), target );
    }

    ImGui::DockBuilderFinish( p_dockspace_id );
}

void Editor::draw_menu_bar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu( "File" )) {
        if (ImGui::MenuItem( "Quit", "Alt+F4" ))
            scene.request_quit();

        for (const auto& command : commands) {
            if (command.menu != "File" || !command.invoke)
                continue;
            if (ImGui::MenuItem( command.label.c_str() ))
                command.invoke();
        }
        ImGui::EndMenu();
    }

    // Plugin menus, in registration order.
    DynamicArray<String> menus_seen;
    for (const auto& command : commands) {
        if (command.menu.empty() || command.menu == "File" || command.menu == "View")
            continue;
        if (std::find( menus_seen.begin(), menus_seen.end(), command.menu ) != menus_seen.end())
            continue;
        menus_seen.push_back( command.menu );

        if (ImGui::BeginMenu( command.menu.c_str() )) {
            for (const auto& item : commands) {
                if (item.menu != command.menu || !item.invoke)
                    continue;
                if (ImGui::MenuItem( item.label.c_str() ))
                    item.invoke();
            }
            ImGui::EndMenu();
        }
    }

    if (ImGui::BeginMenu( "View" )) {
        for (auto& panel : panels)
            ImGui::MenuItem( panel->get_title().c_str(), nullptr, &panel->get_visible() );

        for (const auto& command : commands) {
            if (command.menu != "View" || !command.invoke)
                continue;
            if (ImGui::MenuItem( command.label.c_str() ))
                command.invoke();
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void Editor::draw()
{
    draw_menu_bar();

    for (auto& panel : panels)
        panel->draw( ctx );
}

void Editor::publish_ui_capture()
{
    if (!ctx.inputs)
        return;

    const ImGuiIO& io = ImGui::GetIO();
    ctx.inputs->set_ui_capture( io.WantTextInput, io.WantCaptureMouse && !any_viewport_active() );
}

RID Editor::get_editor_camera()
{
    auto query = scene.get_ecs().query( "EditorCanvas_EditorCamera" ).with<EditorCameraTag, CameraComponent>().build();
    for (auto it : query.entities()) {
        if (auto* c = it.get_component<CameraComponent>())
            return c->source;
    }
    return RID();
}

void Editor::release_gpu()
{
    auto* vid = scene.get_ecs().get_singleton<VideoServices>();
    if (!vid || !vid->renderer)
        return;

    for (auto& panel : panels)
        panel->release_gpu( *vid->renderer );

    if (assets_index)
        assets_index->release_gpu( *vid->renderer );
}

//------------------------------------------------------------------------------
// Editor Logs
//------------------------------------------------------------------------------

namespace {

struct StagedLogLine {
    log::Level level = log::Level::LINFO;
    String channel;
    String text;
};

struct LogMirror {
    std::mutex mutex;
    DynamicArray<StagedLogLine> pending;
};

LogMirror& log_mirror()
{
    static LogMirror* mirror = new LogMirror();
    return *mirror;
}

void clear_staged_logs()
{
    LogMirror& mirror = log_mirror();
    std::lock_guard<std::mutex> lock( mirror.mutex );
    mirror.pending.clear();
}

log::ListenerToken install_log_listener()
{
    clear_staged_logs();
    return log::add_listener( &stage );
}

constexpr int MAX_LOG_LINES   = 20000;
constexpr int LOG_LEVEL_COUNT = 6;

const ImVec4 LOG_LEVEL_COLORS[LOG_LEVEL_COUNT] = {
    ImVec4( 0.55f, 0.55f, 0.55f, 1.0f ), ImVec4( 0.55f, 0.78f, 0.90f, 1.0f ), ImVec4( 0.70f, 0.90f, 0.70f, 1.0f ),
    ImVec4( 0.95f, 0.85f, 0.35f, 1.0f ), ImVec4( 1.00f, 0.45f, 0.40f, 1.0f ), ImVec4( 1.00f, 0.25f, 0.25f, 1.0f ),
};

int log_channel_index( LogStore& store, StringView channel )
{
    for (size_t i = 0; i < store.channels.size(); ++i) {
        if (StringView( store.channels[i] ) == channel)
            return static_cast<int>( i );
    }

    store.channels.push_back( String( channel.data(), channel.size() ) );
    return static_cast<int>( store.channels.size() - 1 );
}

// Appends exactly one display line so LogsOffset, LogsLevel and LogsChannel
// stay in lockstep, one entry per line.
void append_log_line( LogStore& store, const String& text, log::Level level, StringView channel )
{
    const int old_size = store.buffer.size();
    store.buffer.append( text.c_str(), text.c_str() + text.size() );
    store.buffer.append( "\n" );

    if (store.offset.empty())
        store.offset.push_back( 0 );
    for (int i = old_size; i < store.buffer.size(); ++i) {
        if (store.buffer[i] == '\n')
            store.offset.push_back( i + 1 );
    }

    store.level.push_back( static_cast<uint8_t>( level ) );
    store.channel.push_back( log_channel_index( store, channel ) );
}

void trim_log_buffer( LogStore& store )
{
    const int line_count = store.offset.Size > 0 ? store.offset.Size - 1 : 0;
    const int excess     = line_count - MAX_LOG_LINES;
    if (excess <= 0)
        return;

    const int cut = store.offset[excess];
    String remainder( store.buffer.begin() + cut, store.buffer.end() );
    store.buffer.clear();
    store.buffer.append( remainder.c_str(), remainder.c_str() + remainder.size() );

    for (int i = excess; i < store.offset.Size; ++i)
        store.offset[i] -= cut;
    store.offset.erase( store.offset.begin(), store.offset.begin() + excess );

    store.level.erase( store.level.begin(), store.level.begin() + excess );
    store.channel.erase( store.channel.begin(), store.channel.begin() + excess );
}

} // namespace

void stage( const log::LogMsg& msg )
{
    LogMirror& mirror = log_mirror();
    std::lock_guard<std::mutex> lock( mirror.mutex );

    StagedLogLine line;
    line.level   = msg.level;
    line.channel = String( msg.channel.data(), msg.channel.size() );
    line.text    = msg.payload;
    mirror.pending.push_back( std::move( line ) );
}

void Editor::drain_logs()
{
    DynamicArray<StagedLogLine> pending;
    {
        LogMirror& mirror = log_mirror();
        std::lock_guard<std::mutex> lock( mirror.mutex );
        pending.swap( mirror.pending );
    }

    if (pending.empty())
        return;

    for (const auto& line : pending) {
        const StringView triplet = log::level_triplet( line.level );

        String prefix;
        prefix.append( triplet.data(), triplet.size() );
        prefix.append( " " );
        prefix.append( line.channel );
        prefix.append( " | " );

        // Payloads may be multi-line (e.g. the config summary): emit one display
        // line per segment so the per-line metadata stays aligned with offsets.
        size_t begin = 0;
        bool emitted = false;
        for (;;) {
            const auto sep = line.text.find( '\n', begin );
            const auto end = sep == String::npos ? line.text.size() : sep;

            if (emitted && sep == String::npos && end == begin)
                break; // trailing newline produced an empty last segment

            String text = prefix;
            text.append( line.text, begin, end - begin );
            append_log_line( log_store, text, line.level, line.channel );
            emitted = true;

            if (sep == String::npos)
                break;
            begin = sep + 1;
        }
    }

    trim_log_buffer( log_store );
}

void Editor::clear_logs()
{
    log_store.clear();
}

void LogStore::clear()
{
    buffer.clear();
    offset.clear();
    level.clear();
    channel.clear();
    // channels stays: it backs the channel filter's option list.
}

ImVec4 LogStore::level_color( int level )
{
    return LOG_LEVEL_COLORS[std::clamp( level, 0, LOG_LEVEL_COUNT - 1 )];
}

//------------------------------------------------------------------------------
// Selection
//------------------------------------------------------------------------------

namespace {

EcsQuery selection_query( EcsWorld& world )
{
    return world.query( "EditorSelection" ).with<SelectedTag>().build();
}

} // namespace

void Selection::refresh( EcsWorld& world )
{
    selected_eid = INVALID_ENTITY_ID;

    auto query = selection_query( world );
    for (auto ctx : query.entities())
        selected_eid = ctx.entity();
}

void Selection::select( EcsWorld& world, EcsEntity entity )
{
    clear( world );

    if (entity != INVALID_ENTITY_ID)
        world.entity( entity ).add<SelectedTag>( SelectedTag{} ).build();

    selected_eid = entity;
}

void Selection::clear( EcsWorld& world )
{
    DynamicArray<EcsEntity> previous;
    auto query = selection_query( world );
    for (auto ctx : query.entities())
        previous.push_back( ctx.entity() );

    for (auto entity : previous)
        world.remove_component<SelectedTag>( entity );

    selected_eid = INVALID_ENTITY_ID;
}

Node* Selection::node( Scene& scene ) const
{
    if (selected_eid == INVALID_ENTITY_ID)
        return nullptr;
    return scene.node_from_entity( selected_eid );
}

} // namespace nc::editor
