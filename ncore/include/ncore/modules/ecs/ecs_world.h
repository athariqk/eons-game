#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <memory>

#include <ncore/kernel/types.h>
#include <ncore/kernel/world.h>
#include <ncore/utils/logger/log_channel.h>
#include <ncore/utils/macro.h>

#include "ecs_entity.h"
#include "ecs_module.h"
#include "ecs_system.h"

namespace ncore {

class Viewport;
class EventBus;

class EcsWorld : public IWorld {
public:
    EcsWorld();
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
        NC_LOG_TRACE_C(log::ECS, "begin loading ECS module: {}", rfl::Registry::get_class_name<T>());
        T module{};
        module(*this);
        NC_LOG_TRACE_C(log::ECS, "finished loading ECS module");
    }

    /**
     * @brief Returns a new live entity ID.
     */
    EcsEntity create_entity();

    void destroy(EcsEntity entity);

    template<typename T>
    T &get() const {
        return *static_cast<T *>(component_get(EcsEntity{}, rfl::type_id<T>()));
    }

    template<typename T, typename... Args>
    T &set(Args &&...args) {
        auto entity = create_entity();
        T value{std::forward<Args>(args)...};
        component_set(entity, rfl::type_id<T>(), &value, sizeof(T));
        return *static_cast<T *>(component_get(entity, rfl::type_id<T>()));
    }

    template<typename T>
    bool has(const EcsEntity &entity) const {
        return component_has(entity, rfl::type_id<T>());
    }

    template<std::derived_from<EcsSystem> T, typename... TArgs>
    T &add_system(TArgs &&...args) {
        T value{std::forward<TArgs>(args)...};
        system_add(rfl::type_id<T>(), &value);
        return *static_cast<T *>(system_get(rfl::type_id<T>()));
    }

    template<std::derived_from<EcsSystem> T>
    T *get_system() {
        return static_cast<T *>(system_get(rfl::type_id<T>()));
    }

    void set_services(ServiceLocator *locator) { services = locator; }
    ServiceLocator *get_services() const { return services; }

    void set_viewport(Viewport *vp) { viewport = vp; }
    Viewport *get_viewport() const { return viewport; }

    void set_event_bus(EventBus *bus) { event_bus = bus; }
    EventBus *get_event_bus() const { return event_bus; }

private:
    void component_set(EcsEntity eid, rfl::TypeId tid, const void *data, size_t sz);
    void *component_get(EcsEntity eid, rfl::TypeId tid) const;
    bool component_has(EcsEntity eid, rfl::TypeId tid) const;

    void system_add(rfl::TypeId tid, const void *data);
    void *system_get(rfl::TypeId tid) const;

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    ServiceLocator *services = nullptr;
    Viewport *viewport = nullptr;
    EventBus *event_bus = nullptr;
};

} // namespace ncore
