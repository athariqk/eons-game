#include "World.h"

#include <modules/ecs/systems/AudioSystem.h>
#include <modules/ecs/systems/InputSystem.h>
#include <modules/ecs/systems/PhysicsSystem.h>
#include <modules/ecs/systems/RenderSystem.h>
#include <modules/utils/Logger.h>

namespace ncore {

void World::on_init_() {
    // Register foundational systems
    add_system<PhysicsSystem>();
    add_system<InputSystem>();
    add_system<AudioSystem>();
    add_system<RenderSystem>();

    // Initialize all systems
    for (auto &system: systems_reg) {
        if (!system->get_is_enabled())
            continue;
        if (!system->on_init(*this)) {
            LOG_ERROR(log::ECS, "System init FAIL: {}", typeid(*system).name());
        }
    }

    on_init();
}

void World::on_fixed_update_(double p_delta, uint64_t ticks) {
    // Cleanup disabled entities
    for (auto i(0u); i < max_groups; i++) {
        auto &v(grouped_entities_reg[i]);
        std::erase_if(v, [i](Entity *mEntity) { return !mEntity->get_is_enabled() || !mEntity->has_group(i); });
    }
    std::erase_if(entities, [](const std::unique_ptr<Entity> &mEntity) { return !mEntity->get_is_enabled(); });

    // Fixed update systems (physics, gameplay)
    for (auto &system: systems_reg) {
        if (system->get_is_enabled()) {
            system->on_fixed_update(*this, p_delta);
        }
    }
}

void World::on_variable_update_(double p_delta) {
    // Variable update systems (animation, AI, non-critical)
    for (auto &system: systems_reg) {
        if (system->get_is_enabled()) {
            system->on_variable_update(*this, p_delta);
        }
    }

    on_update(p_delta);
}

void World::on_post_update_(double p_delta) {
    for (auto &system: systems_reg) {
        if (system->get_is_enabled()) {
            system->on_post_update(*this, p_delta);
        }
    }
}

void World::on_render_(IGraphicsContext &graphics) {
    // Render systems
    for (auto &system: systems_reg) {
        if (system->get_is_enabled()) {
            system->on_render(*this, graphics);
        }
    }

    on_render(graphics);
}

void World::on_gui_render_() {
    for (auto &system: systems_reg) {
        if (system->get_is_enabled()) {
            system->on_gui_render(*this);
        }
    }

    on_gui_render();
}

void World::on_finish_() {
    entities.clear();

    on_finish();

    // Shutdown systems
    for (auto &system: systems_reg) {
        if (system->get_is_enabled()) {
            system->on_shutdown(*this);
        }
    }
}

void World::add_to_group(Entity *p_entity, Group p_group) { grouped_entities_reg[p_group].emplace_back(p_entity); }

std::vector<Entity *> &World::get_group(Group p_group) { return grouped_entities_reg[p_group]; }

Entity *World::get_entity_by_id(EntityID p_id) {
    auto it = entity_id_reg.find(p_id);
    return (it != entity_id_reg.end()) ? it->second : nullptr;
}

Entity &World::create_entity() {
    auto id = entities.size();
    auto e = new Entity(*this, id);
    std::unique_ptr<Entity> entity_ptr{e};
    entities.emplace_back(std::move(entity_ptr));
    entity_id_reg[id] = e;
    return *e;
}

} // namespace ncore
