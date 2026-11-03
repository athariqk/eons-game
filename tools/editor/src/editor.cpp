#include "editor.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <imgui_internal.h>
#include <mutex>
#include <utility>

#include <ncore/runtime/scene.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/log.h>

#include <io/asset_importer.h>
#include <io/editor_assets.h>
#include <io/thumbnail_cache.h>
#include <panels/asset_picker.h>
#include <panels/assets_panel.h>
#include <panels/inspector_panel.h>
#include <panels/logs_panel.h>
#include <panels/material_inspector.h>
#include <panels/scene_tree_panel.h>
#include <panels/toolbar_panel.h>
#include <panels/viewport_panel.h>

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

Editor::Editor( Scene& p_scene ) : scene( p_scene )
{
    assets_database = std::make_unique<AssetDatabase>();
    importer        = std::make_unique<AssetImporter>();
    thumbnails      = std::make_unique<ThumbnailCache>();

    if (auto* assets_service = p_scene.get_app_ctx()->services.resolve<AssetService>())
        importer->init( *p_scene.get_app_ctx(), p_scene.get_resource_loader(), *assets_service, *assets_database );

    settings.load( DEFAULT_SETTINGS_PATH );

    log_token = install_log_listener();

    create_panels();
}

Editor::~Editor()
{
    inspector_handlers.clear();
    property_widgets.clear();
    panels.clear();

    log_token = {};
    clear_staged_logs();
}

EditorPanel* Editor::register_panel( DockSlot slot, int order, Ptr<EditorPanel> panel )
{
    // The registration call is authoritative: a panel is docked where the
    // plugin asked, not where its constructor happened to default.
    panel->set_slot( slot );
    panel->set_order( order );

    panels.push_back( std::move( panel ) );
    return panels.back().get();
}

void EditorPanel::draw_window( EditorFrame& ctx )
{
    pre_begin( ctx );

    if (is_visible) {
        bool drawn = ImGui::Begin( title.c_str(), is_closable ? &is_visible : nullptr, window_flags() );
        if (drawn) {
            draw_contents( ctx );
        } else {
            on_collapsed( ctx );
        }
        ImGui::End();
    }

    post_end( ctx );
}

void Editor::register_inspector_handler( Ptr<InspectorHandler> handler )
{
    if (handler)
        inspector_handlers.push_back( std::move( handler ) );
}

void Editor::register_property_widget( const types::TypeInfo* type, PropertyWidgetFn widget )
{
    if (!type || !widget)
        return;

    property_widgets.push_back( PropertyWidget{ type, std::move( widget ) } );
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

void Editor::create_panels()
{
    ResourceLoader& loader = scene.get_resource_loader();

    // Toolbar
    register_panel( DockSlot::Top, 0, std::make_unique<ToolbarPanel>( settings ) );

    // Viewports
    auto scene_view = std::make_unique<ViewportPanel>(
        "Scene View", DockSlot::Center, 0, ViewportPanel::CameraSource::EditorCamera, false
    );
    auto* scene_panel = static_cast<ViewportPanel*>( register_panel( DockSlot::Center, 0, std::move( scene_view ) ) );
    scene_panel->add_overlay( std::make_unique<GizmoOverlay>( scene, selection_state, settings ) );
    scene_panel->add_overlay( std::make_unique<StatsOverlay>() );
    register_panel(
        DockSlot::Center, 1,
        std::make_unique<ViewportPanel>(
            "Game View", DockSlot::Center, 1, ViewportPanel::CameraSource::ActiveGameCamera, true
        )
    );

    // Scene + Inspector
    register_panel(
        DockSlot::LeftUpper, 0, std::make_unique<SceneTreePanel>( scene, selection_state, *assets_database )
    );
    register_inspector_handler( std::make_unique<MaterialInspectorHandler>() );
    register_panel(
        DockSlot::RightUpper, 0,
        std::make_unique<InspectorPanel>(
            scene, selection_state, *assets_database, *importer, *thumbnails, loader, inspector_handlers,
            property_widgets
        )
    );

    // Assets
    register_panel(
        DockSlot::LeftLower, 0,
        std::make_unique<AssetsPanel>( *assets_database, *importer, *thumbnails, loader, selection_state )
    );
    // add_asset_property_widget( *this );

    // Logs
    register_panel( DockSlot::Bottom, 0, std::make_unique<LogsPanel>( log_store, true ) );

    sort_panels();
    apply_settings();
}

void Editor::begin_frame()
{
    frame.scene = &scene;

    selection_state.refresh( scene.get_ecs() );
    frame.any_viewport_hovered = false;
    frame.any_viewport_focused = false;

    // the staging buffer must stay bounded even while the console is closed.
    drain_logs();
    importer->pump();
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
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu( "View" )) {
        for (auto& panel : panels)
            ImGui::MenuItem( panel->get_title().c_str(), nullptr, &panel->get_visible() );
        ImGui::EndMenu();
    }

    auto& app_desc = scene.get_app_ctx()->app_desc;

    char version[64];
    std::snprintf(
        version, sizeof version, "%d.%d.%d-%s", app_desc.version.major, app_desc.version.minor, app_desc.version.patch,
        app_desc.version.identifier.c_str()
    );

    ImGui::SameLine();
    const float text_w = ImGui::CalcTextSize( version ).x;
    const float avail  = ImGui::GetContentRegionAvail().x;
    if (avail > text_w)
        ImGui::SetCursorPosX( ImGui::GetCursorPosX() + avail - text_w );
    ImGui::TextDisabled( "%s", version );

    ImGui::EndMainMenuBar();
}

void Editor::draw()
{
    draw_menu_bar();

    for (auto& panel : panels)
        panel->draw_window( frame );
}

void Editor::publish_ui_capture()
{
    auto inputs = scene.get_app_ctx()->services.resolve<InputService>();
    if (!inputs)
        return;

    const ImGuiIO& io = ImGui::GetIO();
    inputs->set_ui_capture( io.WantTextInput, io.WantCaptureMouse && !any_viewport_active() );
}

void Editor::release_gpu()
{
    auto renderer = scene.get_app_ctx()->services.resolve<RenderService>();
    if (!renderer)
        return;

    for (auto& panel : panels)
        panel->release_gpu( *renderer );

    if (thumbnails)
        thumbnails->release_gpu();
}

//------------------------------------------------------------------------------
// Editor Logs
//------------------------------------------------------------------------------

namespace {

struct StagedLogLine {
    log::Level level = log::Level::Info;
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

} // namespace nc::editor
