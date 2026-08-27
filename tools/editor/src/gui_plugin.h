#pragma once

#include <ncore.h>

namespace nc {
class Scene;
}

namespace nc::editor {

void NCAPI register_gui_plugin( Scene& scene );
void NCAPI unregister_gui_plugin( Scene& scene );

} // namespace nc::editor
