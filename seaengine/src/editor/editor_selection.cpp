#include "editor_selection.h"

#include "editor_components.h"

namespace sea::editor {

//------------------------------------------------------------------------------
// EditorSelection
//------------------------------------------------------------------------------

namespace {

nc::EcsQuery selection_query( nc::EcsWorld& world )
{
    return world.query( "EditorSelection" ).with<SelectedTag>().build();
}

} // namespace

void EditorSelection::refresh()
{
    auto& world    = scene.get_ecs();
    const auto old = selection;

    // Drop any cached entity selection and re-read the tag from the world.
    nc::EcsEntity found = nc::INVALID_ENTITY_ID;
    auto query          = selection_query( world );
    for (auto ctx : query.entities())
        found = ctx.entity();

    if (found != nc::INVALID_ENTITY_ID) {
        // The tag wins: an entity selection supersedes an asset selection.
        selection = found;
    } else if (std::holds_alternative<nc::EcsEntity>( selection )) {
        selection = nc::INVALID_ENTITY_ID;
    }

    if (selection != old)
        ++version;
}

void EditorSelection::select_asset( nc::StringView p_path )
{
    selection = nc::String( p_path.data(), p_path.size() );
    ++version;
}

void EditorSelection::select( nc::EcsEntity entity )
{
    clear();

    if (entity != nc::INVALID_ENTITY_ID)
        scene.get_ecs().entity( entity ).add<SelectedTag>( SelectedTag{} ).build();

    selection = entity;
}

void EditorSelection::clear()
{
    previous_sel_entities.reset();

    auto& world = scene.get_ecs();

    auto query = selection_query( world );
    for (auto ctx : query.entities())
        previous_sel_entities.emplace( ctx.entity() );

    for (auto entity : previous_sel_entities)
        world.remove_component<SelectedTag>( entity );

    selection = nc::INVALID_ENTITY_ID;
    ++version;
}

nc::EcsEntity EditorSelection::get_selected_eid() const
{
    const nc::EcsEntity* entity = std::get_if<nc::EcsEntity>( &selection );
    return entity ? *entity : nc::INVALID_ENTITY_ID;
}

nc::Node* EditorSelection::node() const
{
    const nc::EcsEntity* entity = std::get_if<nc::EcsEntity>( &selection );
    if (!entity || *entity == nc::INVALID_ENTITY_ID)
        return nullptr;
    return scene.get_node_from_entity( *entity );
}

nc::StringView EditorSelection::get_selected_asset() const
{
    const nc::String* asset = std::get_if<nc::String>( &selection );
    return asset ? nc::StringView( *asset ) : nc::StringView{};
}

bool EditorSelection::is_asset() const
{
    return std::holds_alternative<nc::String>( selection );
}

/**
 * @brief Bumps whenever the selection changes.
 */
uint64_t EditorSelection::get_version() const
{
    return version;
}

} // namespace sea::editor
