#include "World.h"

#include <AudioSystem.h>
#include <Engine.h>
#include <InputSystem.h>
#include <Logger.h>
#include <PhysicsSystem.h>
#include <RenderSystem.h>

namespace Aeon {

void World::_OnInit() {
    // Register foundational systems
    AddSystem<PhysicsSystem>();
    AddSystem<InputSystem>();
    AddSystem<AudioSystem>();
    AddSystem<RenderSystem>();

    // Initialize all systems
    for (auto &system: m_systems) {
        if (!system->IsEnabled())
            continue;
        if (!system->OnInit(*this)) {
            LOG_ERROR("System {} failed to initialize!", typeid(*system).name());
        }
    }

    OnInit();
}

void World::_OnFixedUpdate(double p_delta, uint64_t ticks) {
    // Cleanup disabled entities
    for (auto i(0u); i < maxGroups; i++) {
        auto &v(groupedEntities[i]);
        std::erase_if(v, [i](Entity *mEntity) { return !mEntity->IsEnabled() || !mEntity->HasGroup(i); });
    }
    std::erase_if(entities, [](const std::unique_ptr<Entity> &mEntity) { return !mEntity->IsEnabled(); });

    // Fixed update systems (physics, gameplay)
    for (auto &system: m_systems) {
        if (system->IsEnabled()) {
            system->OnFixedUpdate(*this, p_delta);
        }
    }
}

void World::_OnVariableUpdate(double p_delta) {
    // Variable update systems (animation, AI, non-critical)
    for (auto &system: m_systems) {
        if (system->IsEnabled()) {
            system->OnVariableUpdate(*this, p_delta);
        }
    }

    OnUpdate(p_delta);
}

void World::_OnPostUpdate(double p_delta) {
    for (auto &system: m_systems) {
        if (system->IsEnabled()) {
            system->OnPostUpdate(*this, p_delta);
        }
    }
}

void World::_OnRender(IGraphicsContext &graphics) {
    // Render systems
    for (auto &system: m_systems) {
        if (system->IsEnabled()) {
            system->OnRender(*this, graphics);
        }
    }

    OnRender(graphics);
}

void World::_OnGuiRender() {
    for (auto &system: m_systems) {
        if (system->IsEnabled()) {
            system->OnGuiRender(*this);
        }
    }

    OnGuiRender();
}

void World::_OnFinish() {
    entities.clear();

    // Shutdown systems
    for (auto &system: m_systems) {
        if (system->IsEnabled()) {
            system->OnShutdown(*this);
        }
    }

    OnFinish();
}

void World::AddToGroup(Entity *mEntity, Group mGroup) { groupedEntities[mGroup].emplace_back(mEntity); }

std::vector<Entity *> &World::GetGroup(Group mGroup) { return groupedEntities[mGroup]; }

Entity *World::GetEntityById(EntityID p_id) {
    auto it = m_entityIdMap.find(p_id);
    return (it != m_entityIdMap.end()) ? it->second : nullptr;
}

Entity &World::CreateEntity() {
    auto id = entities.size();
    auto e = new Entity(*this, id);
    std::unique_ptr<Entity> uPtr{e};
    entities.emplace_back(std::move(uPtr));
    m_entityIdMap[id] = e;
    return *e;
}

} // namespace Aeon
