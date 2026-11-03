#pragma once

#include <editor.h>

namespace nc::editor {

/**
 * @brief Log console view. Ingestion and storage live on Editor (LogStore).
 */
class LogsPanel final : public EditorPanel {
public:
    LogsPanel( LogStore& logs NC_LIFETIME_BOUND, bool visible );

    void draw_contents( EditorFrame& ctx ) override;

private:
    LogStore& logs_;
};

} // namespace nc::editor
