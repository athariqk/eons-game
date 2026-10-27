#include <panels/logs_panel.h>

namespace nc::editor {

LogsPanel::LogsPanel( bool p_visible ) : EditorPanel( "Logs", DockSlot::Bottom, 1, p_visible ) {}

void LogsPanel::draw( EditorContext& p_ctx )
{
    auto* log = p_ctx.log_store;
    if (!log)
        return;

    auto& state = *log;

    if (!ImGui::Begin( title.c_str(), &is_visible )) {
        ImGui::End();
        return;
    }

    if (ImGui::SmallButton( "Clear" ))
        state.clear();

    ImGui::SameLine();
    if (ImGui::SmallButton( "Copy" ))
        ImGui::SetClipboardText( state.buffer.c_str() );

    ImGui::SameLine();
    ImGui::Checkbox( "Auto-scroll", &state.is_autoscroll );

    ImGui::SameLine();
    ImGui::SetNextItemWidth( 180.0f );
    state.filter.Draw( "##logfilter", 180.0f );

    static const char* LEVEL_LABELS[]   = { "T", "D", "I", "W", "E", "F" };
    static const char* LEVEL_TOOLTIPS[] = { "Trace", "Debug", "Info", "Warn", "Error", "Fatal" };
    for (int i = 0; i < 6; ++i) {
        ImGui::SameLine();
        ImGui::PushStyleColor( ImGuiCol_Text, LogStore::level_color( i ) );
        ImGui::Checkbox( LEVEL_LABELS[i], &state.is_level_enabled[i] );
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip( "%s", LEVEL_TOOLTIPS[i] );
    }

    ImGui::SameLine();
    const bool channel_selected =
        state.channel_filter >= 0 && state.channel_filter < static_cast<int>( state.channels.size() );
    const char* channel_preview = channel_selected ? state.channels[state.channel_filter].c_str() : "All channels";
    ImGui::SetNextItemWidth( 140.0f );
    if (ImGui::BeginCombo( "##logchannel", channel_preview )) {
        if (ImGui::Selectable( "All channels", !channel_selected ))
            state.channel_filter = -1;
        for (int i = 0; i < static_cast<int>( state.channels.size() ); ++i) {
            if (ImGui::Selectable( state.channels[i].c_str(), state.channel_filter == i ))
                state.channel_filter = i;
        }
        ImGui::EndCombo();
    }

    // offsets keep a trailing sentinel, here we clamp against
    // the per-line metadata so a mismatch can never index out of range.
    const int line_count =
        std::min( { state.offset.Size > 0 ? state.offset.Size - 1 : 0, state.level.Size, state.channel.Size } );

    bool level_filter_active = false;
    for (bool enabled : state.is_level_enabled)
        level_filter_active = level_filter_active || !enabled;

    const bool text_filter_active    = state.filter.IsActive();
    const bool channel_filter_active = channel_selected;
    const bool any_filter            = text_filter_active || level_filter_active || channel_filter_active;

    const char* buf = state.buffer.begin();

    auto line_visible = [&]( int line_no ) {
        if (!state.is_level_enabled[std::min<int>( state.level[line_no], 5 )])
            return false;
        if (channel_filter_active && state.channel[line_no] != state.channel_filter)
            return false;
        if (text_filter_active) {
            const char* begin = buf + state.offset[line_no];
            const char* end   = buf + state.offset[line_no + 1] - 1;
            if (!state.filter.PassFilter( begin, end ))
                return false;
        }
        return true;
    };

    auto draw_line = [&]( int line_no ) {
        const char* begin = buf + state.offset[line_no];
        const char* end   = buf + state.offset[line_no + 1] - 1;
        ImGui::PushStyleColor( ImGuiCol_Text, LogStore::level_color( std::min<int>( state.level[line_no], 5 ) ) );
        ImGui::TextUnformatted( begin, end );
        ImGui::PopStyleColor();
    };

    ImGui::BeginChild(
        "##log", ImVec2( 0.0f, 0.0f ), ImGuiChildFlags_Borders,
        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar
    );

    if (any_filter) {
        ImVector<int> visible;
        visible.reserve( line_count );
        for (int i = 0; i < line_count; ++i) {
            if (line_visible( i ))
                visible.push_back( i );
        }

        ImGuiListClipper clipper;
        clipper.Begin( visible.Size );
        while (clipper.Step()) {
            for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
                draw_line( visible[r] );
        }
    } else {
        ImGuiListClipper clipper;
        clipper.Begin( line_count );
        while (clipper.Step()) {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; ++line_no)
                draw_line( line_no );
        }
    }

    if (state.is_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY( 1.0f );

    ImGui::EndChild();

    ImGui::End();
}

} // namespace nc::editor
