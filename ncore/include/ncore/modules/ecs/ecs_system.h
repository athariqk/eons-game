#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ncore/kernel/types.h>
#include <ncore/modules/ecs/ecs_query.h>

namespace ncore {

// Currently, we mostly just have wrappers for Flecs C API.
// Just something that we can build upon in the future
// from a standardized base.

class EcsWorld;

/**
 * @brief Defines which pipeline phase a system runs in.
 */
enum class EcsSystemPhase {
    Init,
    PreUpdate,
    FixedUpdate,
    Update,
    PostUpdate,
};

using IterCallback = void (*)(EcsIter &);
using EachCallback = void (*)(EcsIter &, EcsEntityId);

/**
 * @brief EcsSystemBuilder is fluent API for registering EcsWorld-bound ECS systems.
 */
class EcsSystemBuilder {
public:
    EcsSystemBuilder(EcsWorld &world, std::string name);
    ~EcsSystemBuilder();

    // query builder forwarded functions

    template<typename... Comps>
    EcsSystemBuilder &with() {
        qb_.with<Comps...>();
        return *this;
    }

    template<typename First, typename Second>
    EcsSystemBuilder &with_pair() {
        qb_.with_pair<First, Second>();
        return *this;
    }

    template<typename... Comps>
    EcsSystemBuilder &read() {
        qb_.read<Comps...>();
        return *this;
    }

    EcsSystemBuilder &all() {
        qb_.all();
        return *this;
    }

    EcsSystemBuilder &all_read() {
        qb_.all_read();
        return *this;
    }

    // system specific functions

	/**
     * @brief Sets which pipeline phase the system shall run in.
     */
    EcsSystemBuilder &in(EcsSystemPhase phase);

	// TODO: TBI
    EcsSystemBuilder &order(int32_t priority);

    /**
     * @brief Finalise and register the system with the given per-table callback.
     * @return The entity handle of the created system.
     */
    EcsEntityId iter(IterCallback callback);

    /**
     * @brief Finalise and register the system with the given per-entity callback.
     * @return The entity handle of the created system.
     */
    EcsEntityId each(EachCallback callback);

private:
    EcsWorld &world_;
    std::string name;
    EcsQueryBuilder qb_;

    enum class SystemKind { Iter, Each };
    EcsEntityId init_system_(SystemKind kind, void *callback);

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace ncore
