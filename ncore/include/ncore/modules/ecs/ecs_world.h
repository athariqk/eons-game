#pragma once

#include <algorithm>
#include <concepts>
#include <memory>

#include <ncore/kernel/types.h>
#include <ncore/utils/log.h>

#include "ecs_entity.h"
#include "ecs_module.h"
#include "ecs_query.h"
#include "ecs_system.h"

namespace ncore {

// Currently, we're just doing wrappers for Flecs C API.
// Just something that we can build upon in the future
// from a standardized base.

/**
 * @brief EcsWorld is an implementation of an archetype-based ECS architecture.
 */
class NCORE_API EcsWorld : public NObject {
    NCLASS( EcsWorld, NObject )

public:
    EcsWorld();
    ~EcsWorld();

    EcsWorld( const EcsWorld& )            = delete;
    EcsWorld& operator=( const EcsWorld& ) = delete;

    /**
     * @brief Ticks the systems.
     */
    void progress( double delta_time );

    // modules

    template<std::derived_from<EcsFeature> T, typename... TArgs>
    void load_feature( TArgs&&... args )
    {
        NC_LOG_TRACE_C( log::ECS, "load ECS module: {}", rfl::Registry::get_type_name<T>() );
        T module{ std::forward<TArgs>( args )... };
        module( *this );
        NC_LOG_TRACE_C( log::ECS, "load ECS module DONE" );
    }

    // entities

    EcsEntityBuilder create_entity( const std::string& name = std::string() );
    EcsEntityId get_entity( std::string_view name, EcsEntityId parent = INVALID_ENTITY_ID ) const;
    std::string_view get_entity_name( EcsEntityId entity ) const;
    std::span<EcsEntityId> get_entities() const;
    size_t get_entity_count( bool alive = true ) const;
    void destroy_entity( EcsEntityId entity );

    void add_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second );
    bool has_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second ) const;
    void remove_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second );

    // components

    template<typename T>
    T& get_component() const
    {
        auto result = get_component_( EcsEntityId{}, rfl::Registry::find<T>() );
        return *static_cast<T*>( result );
    }

    template<typename T, typename... Args>
    EcsEntityId set_component( Args&&... args )
    {
        auto entity = create_entity_impl_( {} );
        T value{ std::forward<Args>( args )... };
        return set_component_( entity, rfl::Registry::find<T>(), &value, sizeof( T ) );
    }

    template<typename T, typename... Args>
    void set_component( EcsEntityId entity, Args&&... args )
    {
        T value{ std::forward<Args>( args )... };
        set_component_( entity, rfl::Registry::find<T>(), &value, sizeof( T ) );
    }

    template<typename T>
    bool has_component( const EcsEntityId& entity ) const
    {
        return has_component_( entity, rfl::Registry::find<T>() );
    }

    // systems & queries

    /**
     * @brief Returns a fluent builder for registering a stateless system.
     */
    EcsSystemBuilder create_system( std::string_view name );

    /**
     * @brief Returns a fluent builder for creating a cached query.
     */
    EcsQueryBuilder create_query( std::string_view name );

    /**
     * @brief Registers and/or resolves a component type to/from world
     * via the reflection system.
     *
     * @return Its assigned ID.
     */
    EcsComponentId register_component_type( const rfl::TypeInfo* type );

private:
    friend class EcsSystemBuilder;
    friend class EcsQueryBuilder;
    friend class EcsEntityBuilder;
    friend class EcsIter;

    EcsEntityId create_entity_impl_( const std::string& name );
    EcsEntityId set_component_( EcsEntityId eid, const rfl::TypeInfo* type, const void* data, size_t sz );
    void* get_component_( EcsComponentId eid, const rfl::TypeInfo* type ) const;
    bool has_component_( EcsComponentId eid, const rfl::TypeInfo* type ) const;

    void* get_native_handle_() const;
    EcsQuery create_query_( const std::string& name, void* data );

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace ncore
