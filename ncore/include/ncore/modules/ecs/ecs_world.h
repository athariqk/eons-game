#pragma once

#include <algorithm>
#include <concepts>
#include <memory>

#include <ncore/kernel/types.h>
#include <ncore/modules/game_world.h>
#include <ncore/utils/log.h>

#include "ecs_entity.h"
#include "ecs_module.h"
#include "ecs_query.h"
#include "ecs_system.h"

namespace ncore {

class EventBus;
class Viewport;

/**
 * @brief EcsWorld defines the default game world built on top of an
 * archetype-based ECS architecture.
 */
class EcsWorld : public IGameWorld {
    NCLASS(EcsWorld, IGameWorld)

public:
    EcsWorld(ServiceLocator &services);
    ~EcsWorld();

    EcsWorld(const EcsWorld &) = delete;
    EcsWorld &operator=(const EcsWorld &) = delete;

    void on_init() override;
    bool on_fixed_update(double p_delta) override;
    bool on_variable_update(double p_delta) override;
    void on_finish() override;

    // modules

    template<std::derived_from<EcsModule> T, typename... TArgs>
    void load(TArgs &&...args) {
        NC_LOG_TRACE_C(log::ECS, "load ECS module: {}", rfl::Registry::get_type_name<T>());
        T module{std::forward<TArgs>(args)...};
        module(*this);
        NC_LOG_TRACE_C(log::ECS, "load ECS module DONE");
    }

    // entities

    EcsEntityId create_entity(std::string name = std::string());
    std::span<EcsEntityId> get_entities() const;
    void destroy(EcsEntityId entity);

    // components

    template<typename T>
    T &get_component() const {
        auto result = get_component_(EcsEntityId{}, rfl::Registry::find<T>());
        return *static_cast<T *>(result);
    }

    template<typename T, typename... Args>
    EcsEntityId set_component(Args &&...args) {
        auto entity = create_entity();
        T value{std::forward<Args>(args)...};
        return set_component_(entity, rfl::Registry::find<T>(), &value, sizeof(T));
    }

    template<typename T, typename... Args>
    void set_component(EcsEntityId entity, Args &&...args) {
        T value{std::forward<Args>(args)...};
        set_component_(entity, rfl::Registry::find<T>(), &value, sizeof(T));
    }

    template<typename T>
    bool has_component(const EcsEntityId &entity) const {
        return has_component_(entity, rfl::Registry::find<T>());
    }

    // systems & queries

    /**
     * @brief Returns a fluent builder for registering a stateless system.
     */
    EcsSystemBuilder system(std::string_view name);

    /**
     * @brief Returns a fluent builder for creating a cached query.
     */
    EcsQueryBuilder query(std::string_view name);

    /**
     * @brief Registers (or resolves) a custom reflected component type.
     *
     * @return Its assigned ID.
     */
    EcsComponentId register_component_type(const rfl::TypeInfo *type);

    void set_viewport(Viewport *vp) { viewport = vp; }
    Viewport *get_viewport() const { return viewport; }

private:
    friend class EcsSystemBuilder;
    friend class EcsQueryBuilder;
    friend class EcsIter;

    EcsEntityId set_component_(EcsEntityId eid, const rfl::TypeInfo *type, const void *data, size_t sz);
    void *get_component_(EcsComponentId eid, const rfl::TypeInfo *type) const;
    bool has_component_(EcsComponentId eid, const rfl::TypeInfo *type) const;

    void *ecs_world_handle_() const;
    EcsQuery create_query_(const std::string &name, void *qdesc);

    struct Impl;
    std::unique_ptr<Impl> pImpl;
    Viewport *viewport = nullptr;
    bool wants_to_quit = false;
};

} // namespace ncore
