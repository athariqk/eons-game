#include "editor_components.h"

#include "plugins/gui_plugin.h"

namespace nc::editor {

void register_editor_component_types( Scene& scene )
{
    auto& world = scene.get_ecs();

    auto reg = [&world]( const types::TypeInfo* type ) {
        if (type)
            world.register_component_type( type );
    };

    reg( TypeDB::find<EditorPlugin>() );
    reg( TypeDB::find<EditorCameraTag>() );
    reg( TypeDB::find<SelectedTag>() );
    reg( TypeDB::find<GuiStateComponent>() );
}

bool is_internal_component( const types::TypeInfo* type )
{
    if (!type)
        return true;
    if (type == TypeDB::find<NodeRefComponent>() || type == TypeDB::find<SelectedTag>())
        return true;
    // editor-only components are not part of the authored scene.
    return type->name && std::strncmp( type->name, "nc::editor", 9 ) == 0;
}

bool is_editor_internal_node( Node& node )
{
    return node.has_component<EditorCameraTag>();
}

} // namespace nc::editor
