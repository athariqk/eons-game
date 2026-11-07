#include "editor_components.h"

#include "plugins/gui_plugin.h"

namespace sea::editor {

void register_editor_component_types( nc::Scene& scene )
{
    auto& world = scene.get_ecs();

    auto reg = [&world]( const nc::types::TypeInfo* type ) {
        if (type)
            world.register_component_type( type );
    };

    reg( nc::TypeDB::find<EditorPlugin>() );
    reg( nc::TypeDB::find<EditorCameraTag>() );
    reg( nc::TypeDB::find<SelectedTag>() );
    reg( nc::TypeDB::find<EditorGuiComponent>() );
}

bool is_internal_component( const nc::types::TypeInfo* type )
{
    if (!type)
        return true;
    if (type == nc::TypeDB::find<nc::NodeRefComponent>() || type == nc::TypeDB::find<SelectedTag>())
        return true;
    // editor-only components are not part of the authored scene.
    return type->name && std::strncmp( type->name, "sea::editor", 9 ) == 0;
}

bool is_editor_internal_node( nc::Node& node )
{
    return node.has_component<EditorCameraTag>();
}

} // namespace sea::editor
