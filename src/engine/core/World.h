#pragma once
#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <Entity.h>
#include <MainLoop.h>
#include <System.h>

namespace Aeon {

class Gui;
class IGraphicsContext;

/**
 * @brief The World class manages entities and systems, driven by the MainLoop.
 * It serves as the "core" of the ECS architecture
 * and context bridge between the engine and the game.
 */
class World {
public:
    World() {}
    virtual ~World() = default;

    // Internal, to be called by MainLoop
    void _OnInit();
    void _OnFixedUpdate(double p_delta, uint64_t ticks);
    void _OnVariableUpdate(double p_delta);
    void _OnPostUpdate(double p_delta);
    void _OnRender(IGraphicsContext &graphics);
    void _OnGuiRender();
    void _OnFinish();

    // System management
    template<typename T, typename... TArgs>
    T &AddSystem(TArgs &&...args) {
        static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
        auto system = std::make_unique<T>(std::forward<TArgs>(args)...);
        T *ptr = system.get();
        m_systems.emplace_back(std::move(system));
        LOG_TRACE("Added system {} of priority {} to world", typeid(T).name(), ptr->GetPriority());

        // Sort systems by priority (lower priority = earlier execution)
        std::sort(m_systems.begin(), m_systems.end(),
                  [](const auto &a, const auto &b) { return a->GetPriority() < b->GetPriority(); });

        return *ptr;
    }

    template<typename T>
    T *GetSystem() {
        for (auto &system: m_systems) {
            if (auto *ptr = dynamic_cast<T *>(system.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    // Access to the main loop
    void SetMainLoop(MainLoop &mainLoop) { m_mainLoop = &mainLoop; }
    MainLoop &GetMainLoop() {
        if (!m_mainLoop) {
            throw std::runtime_error("MainLoop not set in World!");
        }
        return *m_mainLoop;
    }

    Entity &CreateEntity();
    void AddToGroup(Entity *mEntity, Group mGroup);
    std::vector<Entity *> &GetGroup(Group mGroup);
    inline const auto &GetEntities() const { return entities; }
    Entity *GetEntityById(EntityID p_id);

public:
    // Custom overridables
    virtual void OnInit() = 0;
    virtual void OnUpdate(double delta) = 0;
    virtual void OnRender(IGraphicsContext &graphics) = 0;
    virtual void OnGuiRender() = 0;
    virtual void OnFinish() = 0;

private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::unordered_map<EntityID, Entity *> m_entityIdMap;
    std::array<std::vector<Entity *>, maxGroups> groupedEntities;

    std::vector<std::unique_ptr<System>> m_systems;

    MainLoop *m_mainLoop = nullptr;
};

} // namespace Aeon
