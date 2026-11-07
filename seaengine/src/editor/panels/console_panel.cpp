#include "console_panel.h"

#include <editor/editor.h>

namespace sea::editor {

ConsolePanel::ConsolePanel( LogStore& p_logs, bool p_visible ) :
    EditorPanel( "Console", DockSlot::BOTTOM, 0, p_visible ), logs( p_logs )
{}

void ConsolePanel::draw_contents( EditorFrame& )
{
    if (ImGui::SmallButton( "Clear" ))
        logs.clear();

    ImGui::SameLine();
    if (ImGui::SmallButton( "Copy" ))
        ImGui::SetClipboardText( logs.buffer.c_str() );

    ImGui::SameLine();
    ImGui::Checkbox( "Auto-scroll", &logs.is_autoscroll );

    ImGui::SameLine();
    ImGui::SetNextItemWidth( 180.0f );
    logs.filter.Draw( "##logfilter", 180.0f );

    static const char* LEVEL_LABELS[]   = { "T", "D", "I", "W", "E", "F" };
    static const char* LEVEL_TOOLTIPS[] = { "Trace", "Debug", "Info", "Warn", "ERROR", "Fatal" };
    for (int i = 0; i < 6; ++i) {
        ImGui::SameLine();
        ImGui::PushStyleColor( ImGuiCol_Text, LogStore::level_color( i ) );
        ImGui::Checkbox( LEVEL_LABELS[i], &logs.is_level_enabled[i] );
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip( "%s", LEVEL_TOOLTIPS[i] );
    }

    ImGui::SameLine();
    const bool channel_selected =
        logs.channel_filter >= 0 && logs.channel_filter < static_cast<int>( logs.channels.size() );
    const char* channel_preview = channel_selected ? logs.channels[logs.channel_filter].c_str() : "All channels";
    ImGui::SetNextItemWidth( 140.0f );
    if (ImGui::BeginCombo( "##logchannel", channel_preview )) {
        if (ImGui::Selectable( "All channels", !channel_selected ))
            logs.channel_filter = -1;
        for (int i = 0; i < static_cast<int>( logs.channels.size() ); ++i) {
            if (ImGui::Selectable( logs.channels[i].c_str(), logs.channel_filter == i ))
                logs.channel_filter = i;
        }
        ImGui::EndCombo();
    }

    // offsets keep a trailing sentinel, here we clamp against
    // the per-line metadata so a mismatch can never index out of range.
    const int line_count =
        std::min( { logs.offset.Size > 0 ? logs.offset.Size - 1 : 0, logs.level.Size, logs.channel.Size } );

    bool level_filter_active = false;
    for (bool enabled : logs.is_level_enabled)
        level_filter_active = level_filter_active || !enabled;

    const bool text_filter_active    = logs.filter.IsActive();
    const bool channel_filter_active = channel_selected;
    const bool any_filter            = text_filter_active || level_filter_active || channel_filter_active;

    const char* buf = logs.buffer.begin();

    auto line_visible = [&]( int line_no ) {
        if (!logs.is_level_enabled[std::min<int>( logs.level[line_no], 5 )])
            return false;
        if (channel_filter_active && logs.channel[line_no] != logs.channel_filter)
            return false;
        if (text_filter_active) {
            const char* begin = buf + logs.offset[line_no];
            const char* end   = buf + logs.offset[line_no + 1] - 1;
            if (!logs.filter.PassFilter( begin, end ))
                return false;
        }
        return true;
    };

    auto draw_line = [&]( int line_no ) {
        const char* begin = buf + logs.offset[line_no];
        const char* end   = buf + logs.offset[line_no + 1] - 1;
        ImGui::PushStyleColor( ImGuiCol_Text, LogStore::level_color( std::min<int>( logs.level[line_no], 5 ) ) );
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

    if (logs.is_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY( 1.0f );

    ImGui::EndChild();
}

} // namespace sea::editor
