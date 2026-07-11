#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ncore/kernel/types.h>
#include <ncore/runtime/ecs_query.h>

namespace nc {

// Currently, we mostly just have wrappers for Flecs C API.
// Just something that we can build upon in the future
// from a standardized base.

class EcsWorld;

/**
 * @brief Defines which pipeline phase a system runs in.
 */
enum class EcsSystemPhase {
    Init,
    PreFrame,
    PreUpdate,
    FixedUpdate,
    Update,
    PostUpdate,
    PostFrame
};

namespace EcsCoreEvent {
// Magic numbers are an implementation detail
const EcsEntityId OnAdd          = 300;
const EcsEntityId OnRemove       = 301;
const EcsEntityId OnSet          = 302;
const EcsEntityId OnDelete       = 303;
const EcsEntityId OnDeleteTarget = 304;
const EcsEntityId OnTableCreate  = 305;
const EcsEntityId OnTableDelete  = 306;
} // namespace EcsCoreEvent

enum class EcsCallbackKind {
    Run,
    Each
};

using RunCallback  = void ( * )( QueryContext& );
using EachCallback = void ( * )( QueryContext&, EcsEntityId );

/**
 * @brief EcsSystemBuilder is fluent API for registering EcsWorld-bound systems.
 */
class NCORE_API EcsSystemBuilder {
public:
    EcsSystemBuilder( EcsWorld& world, std::string name );
    ~EcsSystemBuilder();

    EcsSystemBuilder( const EcsSystemBuilder& )            = delete;
    EcsSystemBuilder& operator=( const EcsSystemBuilder& ) = delete;

    // query builder forwarded functions

    template<typename... Comps>
    EcsSystemBuilder& with()
    {
        qb_.with<Comps...>();
        return *this;
    }

    template<typename First, typename Second>
    EcsSystemBuilder& with_pair()
    {
        qb_.with_pair<First, Second>();
        return *this;
    }

    EcsSystemBuilder& up()
    {
        qb_.up();
        return *this;
    }

    EcsSystemBuilder& self()
    {
        qb_.self();
        return *this;
    }

    template<typename... Comps>
    EcsSystemBuilder& read()
    {
        qb_.read<Comps...>();
        return *this;
    }

    EcsSystemBuilder& all()
    {
        qb_.all();
        return *this;
    }

    EcsSystemBuilder& all_read()
    {
        qb_.all_read();
        return *this;
    }

    // system specific functions

    /**
     * @brief Sets which pipeline phase the system shall run in.
     */
    EcsSystemBuilder& in( EcsSystemPhase phase );

    /**
     * @brief Sets the execution order of the system within the pipeline phase.
     */
    EcsSystemBuilder& order( int32_t priority );

    /**
     * @brief Finalise and register the system with the given per-entity-group callback.
     * @return The entity handle of the created system.
     */
    EcsEntityId run( RunCallback callback );

    /**
     * @brief Finalise and register the system with the given per-entity callback.
     * @return The entity handle of the created system.
     */
    EcsEntityId each( EachCallback callback );

private:
    EcsWorld& world_;
    std::string name;
    EcsQueryBuilder qb_;

    EcsEntityId init_system_( EcsCallbackKind kind, void* callback );

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

class NCORE_API EcsObserverBuilder {
public:
    EcsObserverBuilder( EcsWorld& world, std::string name );
    ~EcsObserverBuilder();

    EcsObserverBuilder( const EcsObserverBuilder& )            = delete;
    EcsObserverBuilder& operator=( const EcsObserverBuilder& ) = delete;

    template<class... Comps>
    EcsObserverBuilder& on( EcsEntityId evt )
    {
        qb_.with<Comps...>();
        events.push_back( evt );
        return *this;
    }

    /**
     * @brief Add query terms without setting the event.
     */
    template<typename... Comps>
    EcsObserverBuilder& with()
    {
        qb_.with<Comps...>();
        return *this;
    }

    /**
     * @brief Set the event(s) this observer listens on (separate from terms).
     */
    EcsObserverBuilder& event( EcsEntityId evt )
    {
        events.push_back( evt );
        return *this;
    }

    EcsObserverBuilder& up()
    {
        qb_.up();
        return *this;
    }

    EcsObserverBuilder& self()
    {
        qb_.self();
        return *this;
    }

    EcsEntityId run( RunCallback callback );

    EcsEntityId each( EachCallback callback );

private:
    EcsWorld& world_;
    std::string name;
    Vector<EcsEntityId> events;
    EcsQueryBuilder qb_;
    EcsEntityId init_observer_( EcsCallbackKind kind, void* callback );
};

} // namespace nc
