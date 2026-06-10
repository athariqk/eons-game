/* Adapted from: https://github.com/carlbirch/BirchEngine */

#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "Definitions.h"

namespace ncore {

class World;

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

class Entity {
public:
    Entity(World &p_world, EntityID p_id) : world(p_world), id(p_id) {}

    bool is_enabled() const { return enabled; }
    void set_enabled(bool p_enabled) { enabled = p_enabled; }
    void destroy() { enabled = false; }

    bool has_group(Group p_group) const { return group_bitset[p_group]; }
    void add_group(Group p_group);
    void remove_group(Group p_group) { group_bitset[p_group] = false; }

    template<typename T>
    bool has_component() const {
        return component_bit_set[getComponentTypeID<T>()];
    }

    template<typename T, typename... TArgs>
    T &add_component(TArgs &&...p_args) {
        T *c(new T(std::forward<TArgs>(p_args)...));
        c->entity = this;
        std::unique_ptr<Component> u_ptr{c};
        components.emplace_back(std::move(u_ptr));

        component_array[getComponentTypeID<T>()] = c;
        component_bit_set[getComponentTypeID<T>()] = true;

        return *c;
    }

    template<typename T>
    T &get_component() const {
        auto ptr = component_array[getComponentTypeID<T>()];
        assert(ptr != nullptr && "Entity does not have requested component");
        return *static_cast<T *>(ptr);
    }

    template<typename T>
    void remove_component() {
        auto cid = getComponentTypeID<T>();
        if (!component_bit_set[cid])
            return;

        component_array[cid] = nullptr;
        component_bit_set[cid] = false;

        components.erase(std::remove_if(components.begin(), components.end(),
                                        [cid](const std::unique_ptr<Component> &comp) {
                                            return dynamic_cast<T *>(comp.get()) != nullptr;
                                        }),
                         components.end());
    }

    World &get_world() { return world; }

    EntityID get_id() const { return id; }

private:
    EntityID id;
    bool enabled = true;
    World &world;
    std::vector<std::unique_ptr<Component>> components;
    ComponentArray component_array{};
    ComponentBitSet component_bit_set{};
    GroupBitset group_bitset{};
};

} // namespace ncore
