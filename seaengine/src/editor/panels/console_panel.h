#pragma once

#include "editor_panel.h"

namespace sea::editor {

struct LogStore;

/**
 * @brief Log console view. Ingestion and storage live on Editor (LogStore).
 */
class ConsolePanel final : public EditorPanel {
public:
    ConsolePanel( LogStore& p_logs NC_LIFETIME_BOUND, bool p_visible );

    void draw_contents( EditorFrame& frame ) override;

private:
    LogStore& logs;
};

} // namespace sea::editor
