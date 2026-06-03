/* Adapted from: https://github.com/carlbirch/BirchEngine */

#pragma once

#include <array>
#include <bitset>
#include <cassert>
#include <memory>
#include <vector>

#include "Component.h"

namespace Aeon {

class World;

using EntityID = std::size_t;
using ComponentID = std::size_t;
using Group = std::size_t;

inline ComponentID getNewComponentTypeID() {
    static ComponentID lastID = 0u;
    return lastID++;
}

template<typename T>
inline ComponentID getComponentTypeID() noexcept {
    static_assert(std::is_base_of_v<Component, T>, "");
    static ComponentID typeID = getNewComponentTypeID();
    return typeID;
}

constexpr std::size_t maxComponents = 32;
constexpr std::size_t maxGroups = 32;

using ComponentBitSet = std::bitset<maxComponents>;
using GroupBitset = std::bitset<maxGroups>;
using ComponentArray = std::array<Component *, maxComponents>;

class Entity {
public:
    Entity(World &p_world, EntityID p_id) : m_world(p_world), m_id(p_id) {}

    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    void Destroy() { m_enabled = false; }

    bool HasGroup(Group mGroup) const { return m_groupBitset[mGroup]; }
    void AddGroup(Group mGroup);
    void RemoveGroup(Group mGroup) { m_groupBitset[mGroup] = false; }

    template<typename T>
    bool HasComponent() const {
        return m_componentBitSet[getComponentTypeID<T>()];
    }

    template<typename T, typename... TArgs>
    T &AddComponent(TArgs &&...mArgs) {
        T *c(new T(std::forward<TArgs>(mArgs)...));
        c->entity = this;
        std::unique_ptr<Component> uPtr{c};
        m_components.emplace_back(std::move(uPtr));

        m_componentArray[getComponentTypeID<T>()] = c;
        m_componentBitSet[getComponentTypeID<T>()] = true;

        return *c;
    }

    template<typename T>
    T &GetComponent() const {
        auto ptr = m_componentArray[getComponentTypeID<T>()];
        assert(ptr != nullptr && "Entity does not have requested component");
        return *static_cast<T *>(ptr);
    }

    template<typename T>
    void RemoveComponent() {
        auto id = getComponentTypeID<T>();
        if (!m_componentBitSet[id])
            return;

        m_componentArray[id] = nullptr;
        m_componentBitSet[id] = false;

        // Remove from vector
        m_components.erase(std::remove_if(m_components.begin(), m_components.end(),
                                          [id](const std::unique_ptr<Component> &comp) {
                                              return dynamic_cast<T *>(comp.get()) != nullptr;
                                          }),
                           m_components.end());
    }

    World &GetWorld() { return m_world; }

    EntityID GetID() const { return m_id; }

private:
    World &m_world;
    bool m_enabled = true;
    EntityID m_id;

    std::vector<std::unique_ptr<Component>> m_components;
    ComponentArray m_componentArray{};
    ComponentBitSet m_componentBitSet{};
    GroupBitset m_groupBitset{};
};

} // namespace Aeon
