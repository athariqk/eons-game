#include "editor_selection.h"

#include "editor_components.h"

namespace nc::editor {

//------------------------------------------------------------------------------
// EditorSelection
//------------------------------------------------------------------------------

namespace {

EcsQuery selection_query( EcsWorld& world )
{
    return world.query( "EditorSelection" ).with<SelectedTag>().build();
}

} // namespace

void EditorSelection::refresh( EcsWorld& world )
{
    const auto old = selection;

    // Drop any cached entity selection and re-read the tag from the world.
    EcsEntity found = INVALID_ENTITY_ID;
    auto query      = selection_query( world );
    for (auto ctx : query.entities())
        found = ctx.entity();

    if (found != INVALID_ENTITY_ID) {
        // The tag wins: an entity selection supersedes an asset selection.
        selection = found;
    } else if (std::holds_alternative<EcsEntity>( selection )) {
        selection = INVALID_ENTITY_ID;
    }

    if (selection != old)
        ++version;
}

void EditorSelection::select_asset( StringView p_path )
{
    selection = String( p_path.data(), p_path.size() );
    ++version;
}

void EditorSelection::select( EcsWorld& world, EcsEntity entity )
{
    clear( world );

    if (entity != INVALID_ENTITY_ID)
        world.entity( entity ).add<SelectedTag>( SelectedTag{} ).build();

    selection = entity;
}

void EditorSelection::clear( EcsWorld& world )
{
    DynamicArray<EcsEntity> previous;
    auto query = selection_query( world );
    for (auto ctx : query.entities())
        previous.push_back( ctx.entity() );

    for (auto entity : previous)
        world.remove_component<SelectedTag>( entity );

    selection = INVALID_ENTITY_ID;
    ++version;
}

Node* EditorSelection::node( Scene& scene ) const
{
    const EcsEntity* entity = std::get_if<EcsEntity>( &selection );
    if (!entity || *entity == INVALID_ENTITY_ID)
        return nullptr;
    return scene.get_node_from_entity( *entity );
}

} // namespace nc::editor
