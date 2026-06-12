/* Adapted from: https://github.com/carlbirch/BirchEngine */

#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include <Definitions.h>

namespace ncore {

class World;

inline ComponentID get_new_component_type_id() {
    static ComponentID last_id = 0u;
    return last_id++;
}

template<typename T>
inline ComponentID get_component_type_id() noexcept {
    static_assert(std::is_base_of_v<Component, T>, "");
    static ComponentID type_id = get_new_component_type_id();
    return type_id;
}

class Entity {
public:
    Entity(World &p_world, EntityID p_id) : world(p_world), id(p_id) {}

    bool get_is_enabled() const { return is_enabled; }
    void set_is_enabled(bool p_enabled) { is_enabled = p_enabled; }
    void destroy() { is_enabled = false; }

    bool has_group(Group p_group) const { return group_bitset[p_group]; }
    void add_group(Group p_group);
    void remove_group(Group p_group) { group_bitset[p_group] = false; }

    template<typename T>
    bool has_component() const {
        return component_bit_set[get_component_type_id<T>()];
    }

    template<typename T, typename... TArgs>
    T &add_component(TArgs &&...p_args) {
        T *c(new T(std::forward<TArgs>(p_args)...));
        c->entity = this;
        std::unique_ptr<Component> u_ptr{c};
        components.emplace_back(std::move(u_ptr));

        component_array[get_component_type_id<T>()] = c;
        component_bit_set[get_component_type_id<T>()] = true;

        return *c;
    }

    template<typename T>
    T &get_component() const {
        auto ptr = component_array[get_component_type_id<T>()];
        assert(ptr != nullptr && "Entity does not have requested component");
        return *static_cast<T *>(ptr);
    }

    template<typename T>
    void remove_component() {
        auto cid = get_component_type_id<T>();
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
    bool is_enabled = true;
    World &world;
    std::vector<std::unique_ptr<Component>> components;
    ComponentArray component_array{};
    ComponentBitSet component_bit_set{};
    GroupBitset group_bitset{};
};

} // namespace ncore
