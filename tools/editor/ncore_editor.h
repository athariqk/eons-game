#pragma once

#include <ncore.hpp>

namespace nc {
class Scene;
}

namespace nc::editor {

void NCAPI register_editor_plugin( Scene& scene );

} // namespace nc::editor
