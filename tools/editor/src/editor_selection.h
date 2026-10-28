#pragma once

#include <variant>

#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/scene.h>

namespace nc::editor {

/**
 * @brief The editor selection: ECS-backed, with the current id cached.
 *
 * SelectedTag stays in the world so engine-side systems can read it; this class
 * is the editor-facing API. A selection is exactly one of an entity or an asset
 * path, modeled as a variant so the two can never be set simultaneously.
 */
class EditorSelection {
public:
    /** @brief Re-read the tag (picks up selection changes made outside the editor). */
    void refresh( EcsWorld& world );
    void select( EcsWorld& world, EcsEntity entity );
    void clear( EcsWorld& world );

    EcsEntity get_selected_eid() const
    {
        const EcsEntity* entity = std::get_if<EcsEntity>( &selection );
        return entity ? *entity : INVALID_ENTITY_ID;
    }

    Node* node( Scene& scene ) const;

    /** @brief Select an asset; the entity selection is dropped (one selection). */
    void select_asset( StringView p_path );

    StringView get_selected_asset() const
    {
        const String* asset = std::get_if<String>( &selection );
        return asset ? StringView( *asset ) : StringView{};
    }

    bool is_asset() const
    {
        return std::holds_alternative<String>( selection );
    }

    /**
     * @brief Bumps whenever the selection changes.
     */
    uint64_t get_version() const
    {
        return version;
    }

private:
    std::variant<EcsEntity, String> selection = INVALID_ENTITY_ID;
    uint64_t version                           = 0;
};

} // namespace nc::editor
