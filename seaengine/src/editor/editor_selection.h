#pragma once

#include <variant>

#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/scene.h>

namespace sea::editor {

/**
 * @brief The editor selection: ECS-backed, with the current id cached.
 *
 * SelectedTag stays in the world so engine-side systems can read it; this class
 * is the editor-facing API. A selection is exactly one of an entity or an asset
 * path, modeled as a variant so the two can never be set simultaneously.
 */
class EditorSelection {
public:
    explicit EditorSelection( nc::Scene& p_scene NC_LIFETIME_BOUND ) : scene( p_scene ) {}

    void refresh();
    void select( nc::EcsEntity entity );
    void clear();

    nc::EcsEntity get_selected_eid() const;
    nc::StringView get_selected_asset() const;

    nc::Node* node() const;

    void select_asset( nc::StringView p_path );
    bool is_asset() const;

    /**
     * @brief Bumps whenever the selection changes.
     */
    uint64_t get_version() const;

private:
    nc::Scene& scene;
    std::variant<nc::EcsEntity, nc::String> selection = nc::INVALID_ENTITY_ID;
    uint64_t version                                  = 0;
    nc::BumpAllocator<nc::EcsEntity> previous_sel_entities;
};

} // namespace sea::editor
