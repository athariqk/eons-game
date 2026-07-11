#pragma once

#include <algorithm>
#include <concepts>
#include <memory>

#include <ncore/kernel/types.h>
#include <ncore/utils/log.h>

#include "ecs_entity.h"
#include "ecs_feature.h"
#include "ecs_query.h"
#include "ecs_system.h"

namespace nc {

class IGameWorld;

// Currently, we're just doing wrappers for Flecs C API.
// Just something that we can build upon in the future
// from a standardized base.

/**
 * @brief EcsWorld is an implementation of an archetype-based ECS architecture.
 */
class NCORE_API EcsWorld : public NcObject {
    NCLASS( EcsWorld, NcObject )

public:
    EcsWorld( IGameWorld& game_world );
    ~EcsWorld() override;

    EcsWorld( const EcsWorld& )            = delete;
    EcsWorld& operator=( const EcsWorld& ) = delete;

    /**
     * @brief Ticks the systems.
     */
    void progress( double delta_time );

    // FEATURES

    template<std::derived_from<EcsFeature> T, class... TArgs>
    void load_feature( TArgs&&... args )
    {
        auto name = rtti::TypeRegistry::get_type_name<T>();
        NC_LOG_DEBUG_C( log::ECS, "Loading ECS module: {}", name );
        T module{ std::forward<TArgs>( args )... };
        module( *this );
        NC_LOG_DEBUG_C( log::ECS, "{} loading complete", name );
    }

    // ENTITIES

    EcsEntityBuilder create_entity( const std::string& name = std::string() );
    EcsEntityId get_entity( std::string_view name, EcsEntityId parent = INVALID_ENTITY_ID ) const;
    std::string_view get_entity_name( EcsEntityId entity ) const;
    std::span<EcsEntityId> get_entities() const;
    size_t get_entity_count( bool alive = true ) const;
    void destroy_entity( EcsEntityId entity );

    void add_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second );
    bool has_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second ) const;
    void remove_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second );

    // COMPONENTS

    /**
     * @brief Registers and/or resolves a component type to/from world
     * via NCORE's RTTI.
     *
     * @return Its entity ID (each component type is its own dedicated entity).
     */
    EcsEntityId register_component_type( const rtti::TypeInfo* type );

    /**
     * @return True if EcsWorld has a singleton component of given ID.
     */
    template<class T>
    bool has_component( EcsComponentId id ) const
    {
        return has_component_( id, rtti::TypeRegistry::find<T>() );
    }

    template<class T>
    EcsEntityId resolve_component() const
    {
        auto type = rtti::TypeRegistry::find<T>();
        return register_component_type( type );
    }

    template<class T>
    void remove_component( EcsEntityId eid )
    {
        const rtti::TypeInfo* type = rtti::TypeRegistry::find<T>();
        return remove_component_( eid, type );
    }

    /**
     * @brief Return a singleton component of type T owned by EcsWorld.
     */
    template<class T>
    const T& get_singleton() const
    {
        auto result = get_component_ro_( INVALID_ENTITY_ID, rtti::TypeRegistry::find<T>() );
        NC_ASSERT_NULL( result );
        return *static_cast<const T*>( result );
    }

    /**
     * @brief Create and set a singleton component owned by EcsWorld using
     * copy semantics.
     *
     * This is not for setting a component value for a particular entity,
     * for that use create_entity() builder.
     *
     * @return The component's handle.
     */
    template<class T>
    EcsComponentId set_singleton( const T& value )
    {
        const rtti::TypeInfo* type = rtti::TypeRegistry::find<T>();
        return set_component_( INVALID_ENTITY_ID, type, &value );
    }

    /**
     * @brief Create and set a singleton component owned by EcsWorld in-place.
     *
     * This is not for setting a component's value for a particular entity,
     * for that use create_entity() builder.
     *
     * @return The component's handle.
     */
    template<class T, class... Args>
    EcsComponentId emplace_singleton( Args&&... args )
    {
        const rtti::TypeInfo* type = rtti::TypeRegistry::find<T>();
        // TODO: handle args
        return emplace_component_( INVALID_ENTITY_ID, type );
    }

    // SYSTEMS & QUERIES

    /**
     * @brief Returns a fluent builder for registering a stateless system.
     */
    EcsSystemBuilder create_system( std::string_view name );

    /**
     * @brief Returns a fluent builder for creating a cached query.
     */
    EcsQueryBuilder create_query( std::string_view name );

    EcsObserverBuilder create_observer( std::string_view name );

    /**
     * @brief Sorts systems within each pipeline phase by their order() value
     * and chains them with EcsDependsOn. Call after all features are loaded.
     */
    void finalize_ordering();

    IGameWorld& get_parent()
    {
        return g_world;
    }

    /**
     * @brief Returns the raw implementation world handle.
     */
    void* get_native_handle() const;

private:
    friend class EcsSystemBuilder;
    friend class EcsObserverBuilder;
    friend class EcsQueryBuilder;
    friend class EcsEntityBuilder;
    friend class QueryContext;

    EcsEntityId create_entity_impl_( const std::string& name );
    EcsComponentId set_component_( EcsEntityId eid, const rtti::TypeInfo* type, const void* data );
    EcsComponentId emplace_component_( EcsEntityId eid, const rtti::TypeInfo* type );
    void* get_component_( EcsEntityId id, const rtti::TypeInfo* type ) const;          // returns mutable ptr
    const void* get_component_ro_( EcsEntityId id, const rtti::TypeInfo* type ) const; // returns const ptr, no staging
    bool has_component_( EcsEntityId id, const rtti::TypeInfo* type ) const;
    void remove_component_( EcsEntityId, const rtti::TypeInfo* type ) const;
    EcsQuery create_query_( const std::string& name, void* data );

    struct Impl;
    std::unique_ptr<Impl> pImpl;

    IGameWorld& g_world;
};

} // namespace nc
