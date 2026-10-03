#pragma once

#include <editor/ncore_editor.h>

namespace nc {
class Scene;
}

namespace nc::editor {

void NCAPI_EDITOR register_gui_plugin( Scene& scene );
void NCAPI_EDITOR unregister_gui_plugin( Scene& scene );

} // namespace nc::editor
