#pragma once

#include <algorithm>
#include <concepts>
#include <memory>

#include <ncore/kernel/types.h>
#include <ncore/utils/log.h>

#include "ecs/ecs_entity.h"
#include "ecs/ecs_module.h"
#include "ecs/ecs_system.h"
#include "game_world.h"

namespace ncore {

class EventBus;
class ServiceLocator;
class Viewport;

/**
 * @brief EcsWorld defines a default implementation of the game world specific for NCORE games.
 *
 * This world is built on top of the ECS architecture.
 */
class EcsWorld : public IGameWorld {
    NCLASS(EcsWorld, IGameWorld)

public:
    EcsWorld(EventBus &event_bus, ServiceLocator &services);
    ~EcsWorld();

    EcsWorld(const EcsWorld &) = delete;
    EcsWorld &operator=(const EcsWorld &) = delete;

    void on_init() override;
    void on_fixed_update(double p_delta, uint64_t ticks) override;
    void on_variable_update(double p_delta) override;
    void on_post_update(double p_delta) override;
    void on_finish() override;

    template<std::derived_from<EcsModule> T>
    void load() {
        NC_LOG_TRACE_C(log::ECS, "begin loading ECS module: {}", rfl::Registry::get_type_name<T>());
        T module{};
        module(*this);
        NC_LOG_TRACE_C(log::ECS, "finished loading ECS module");
    }

    /**
     * @brief Returns a new live entity ID.
     */
    EcsEntity create_entity();

    void destroy(EcsEntity entity);

    // TODO: have a way to register components into the reflection system here

    template<typename T>
    T &get() const {
        return *static_cast<T *>(component_get(EcsEntity{}, rfl::Registry::find<T>()));
    }

    template<typename T, typename... Args>
    T &set(Args &&...args) {
        auto entity = create_entity();
        T value{std::forward<Args>(args)...};
        component_set(entity, rfl::Registry::find<T>(), &value, sizeof(T));
        return *static_cast<T *>(component_get(entity, rfl::Registry::find<T>()));
    }

    template<typename T, typename... Args>
    T &add(EcsEntity entity, Args &&...args) {
        T value{std::forward<Args>(args)...};
        component_set(entity, rfl::Registry::find<T>(), &value, sizeof(T));
        return *static_cast<T *>(component_get(entity, rfl::Registry::find<T>()));
    }

    template<typename T>
    bool has(const EcsEntity &entity) const {
        return component_has(entity, rfl::Registry::find<T>());
    }

    template<std::derived_from<EcsSystem> T, typename... TArgs>
    T &add_system(TArgs &&...args) {
        T value{std::forward<TArgs>(args)...};
        system_add(rfl::Registry::find<T>(), &value);
        return *static_cast<T *>(system_get(rfl::Registry::find<T>()));
    }

    template<std::derived_from<EcsSystem> T>
    T *get_system() {
        return static_cast<T *>(system_get(rfl::Registry::find<T>()));
    }

    ServiceLocator &get_services() { return services; }
    EventBus &get_event_bus() { return event_bus; }

    void set_viewport(Viewport *vp) { viewport = vp; }
    Viewport *get_viewport() const { return viewport; }

private:
    void component_set(EcsEntity eid, const rfl::TypeInfo *type, const void *data, size_t sz);
    void *component_get(EcsEntity eid, const rfl::TypeInfo *type) const;
    bool component_has(EcsEntity eid, const rfl::TypeInfo *type) const;

    void system_add(const rfl::TypeInfo *type, const void *data);
    void *system_get(const rfl::TypeInfo *type) const;

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    ServiceLocator &services;
    EventBus &event_bus;
    Viewport *viewport = nullptr;
};

} // namespace ncore
