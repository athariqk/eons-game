#pragma once

#include <ncore/core/collection.h>

namespace nc::editor {

/**
 * @brief Launches platform-specific file explorer at the specified path.
 */
void reveal_in_explorer( StringView p_path );

} // namespace nc::editor
