#pragma once

#include <ncore/core/collection.h>

namespace sea::editor {

/**
 * @brief Launches platform-specific file explorer at the specified path.
 */
void reveal_in_explorer( nc::StringView p_path );

} // namespace sea::editor
