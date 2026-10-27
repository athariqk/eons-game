#pragma once

#include <editor.h>

namespace nc::editor {

/**
 * @brief Log console view. Ingestion and storage live on Editor (LogStore).
 */
class LogsPanel final : public EditorPanel {
public:
    explicit LogsPanel( bool visible );

    void draw( EditorContext& ctx ) override;
};

} // namespace nc::editor
