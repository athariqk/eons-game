#pragma once

#include <memory>

#include <ncore/core/object.h>
#include <ncore/core/types.h>

#include "ecs_entity.h"
#include "ecs_query.h"
#include "ecs_system.h"

namespace nc {

class IGameWorld;
class Node;

/**
 * @brief EcsWorld is an implementation of archetype-based ECS architecture.
 *
 * Currently, we're just doing wrappers over Flecs C API.
 * Just something that we can build upon in the future
 * from a standardized base.
 *
 * Another justification for this wrapper is that, we can keep the logic for
 * handling ECS component registration, lookups etc which leverages our
 * existing RTTI system all in one place.
 */
class NCAPI EcsWorld : public NcObject {
    NCLASS( EcsWorld, NcObject )

public:
    EcsWorld();
    ~EcsWorld() override;

    EcsWorld( const EcsWorld& )            = delete;
    EcsWorld& operator=( const EcsWorld& ) = delete;

    /**
     * @brief Tick all systems.
     */
    void progress( double delta_time );

    // Entities

    /**
     * @brief Create a new entity.
     * @return A fluent builder for registering named entity.
     */
    EcsEntityBuilder entity( const String& name = String() );
    /**
     * @brief Lookup entity info by its name.
     * @param parent If a valid ID, then a child entity of this entity will be searched.
     */
    EcsEntity lookup( StringView entity_name, EcsEntity parent = INVALID_ENTITY_ID ) const;
    /**
     * @brief Lookup entity name by its ID.
     */
    StringView lookup( EcsEntity entity ) const;
    /**
     * @brief Return all alive entities.
     */
    Span<EcsEntity> get_entities() const;
    size_t get_entity_count( bool alive = true ) const;
    void destroy_entity( EcsEntity entity );

    void add_pair( EcsEntity entity, EcsComponent first, EcsComponent second );
    bool has_pair( EcsEntity entity, EcsComponent first, EcsComponent second ) const;
    void remove_pair( EcsEntity entity, EcsComponent first, EcsComponent second );

    void emit_event( EcsEntity event, EcsEntity target );

    // Components

    /**
     * @brief Registers and/or resolves a component type to/from world
     * via NCORE's RTTI system.
     *
     * @return Its entity ID (each component type is its own dedicated entity).
     */
    EcsEntity register_component_type( const rtti::TypeInfo* type ) const;

    /**
     * @return True if EcsWorld has a singleton component of given ID.
     */
    template<class T>
    bool has_component( EcsComponent id ) const
    {
        return has_component_( id, rtti::TypeRegistry::find<T>() );
    }

    template<class T>
    EcsEntity resolve_component() const
    {
        auto type = rtti::TypeRegistry::find<T>();
        return register_component_type( type );
    }

    const rtti::TypeInfo* resolve_component( EcsComponent id ) const;

    /**
     * @brief Get all component types previously registered via register_component_type().
     *
     * @return Non-owning list of component TypeInfo.
     */
    Span<const rtti::TypeInfo*> get_component_types() const;

    template<class T>
    void remove_component( EcsEntity eid )
    {
        const rtti::TypeInfo* type = rtti::TypeRegistry::find<T>();
        return remove_component_( eid, type );
    }

    /**
     * @brief Return a read-only singleton component of type T owned by EcsWorld.
     */
    template<class T>
    const T* get_singleton() const
    {
        auto result = get_component_const_( INVALID_ENTITY_ID, rtti::TypeRegistry::find<T>() );
        return static_cast<const T*>( result );
    }

    /**
     * @brief Return a read-write singleton component of type T owned by EcsWorld.
     */
    template<class T>
    T* get_singleton()
    {
        auto result = get_component_( INVALID_ENTITY_ID, rtti::TypeRegistry::find<T>() );
        return static_cast<T*>( result );
    }

    /**
     * @brief Create/set a singleton component from the given value using
     * copy semantics.
     *
     * This is not for setting a component value for a particular entity,
     * for that use entity() builder.
     *
     * @return The handle to the EcsWorld-owned singleton component.
     */
    template<class T>
    EcsComponent set_singleton( const T& value )
    {
        auto type = rtti::TypeRegistry::find<T>();
        return set_component_( INVALID_ENTITY_ID, type, &value );
    }

    /**
     * @brief Create in-place a singleton component owned by EcsWorld.
     * @return The typed pointer to the constructed singleton component.
     */
    template<class T>
    T* emplace_singleton()
    {
        auto type = rtti::TypeRegistry::find<T>();
        return reinterpret_cast<T*>( emplace_component_( INVALID_ENTITY_ID, type ) );
    }

    template<class T>
    void emit_event( const T& data, EcsEntity target ) const
    {
        auto type = rtti::TypeRegistry::find<T>();
        emit_event_( type, target, &data );
    }

    // Systems/Queries

    /**
     * @brief Create a new system.
     * @return A fluent builder for registering a stateless system.
     */
    EcsSystemBuilder system( StringView name );

    /**
     * @brief Create a new query.
     * @return A fluent builder for registering a cached query.
     */
    EcsQueryBuilder query( StringView name );

    void remove_query( StringView name );

    /**
     * @brief Create a new event observer.
     * @return A fluent builder for registering an event observer.
     */
    EcsObserverBuilder observer( StringView name );

    /**
     * @brief Sorts systems within each pipeline phase by their order() value
     * and chains them with EcsDependsOn. Call after all features are loaded.
     */
    void finalize_ordering();

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
    friend class Node; // FIXME: kinda hacky, lets remove later

    EcsEntity create_entity_impl_( const String& name ) const;
    EcsComponent set_component_( EcsEntity eid, const rtti::TypeInfo* type, const void* data );
    void* emplace_component_( EcsEntity eid, const rtti::TypeInfo* type );
    void* get_component_( EcsEntity id, const rtti::TypeInfo* type ) const;             // returns mutable ptr
    const void* get_component_const_( EcsEntity id, const rtti::TypeInfo* type ) const; // returns const ptr, no staging
    bool has_component_( EcsEntity id, const rtti::TypeInfo* type ) const;
    void remove_component_( EcsEntity, const rtti::TypeInfo* type ) const;
    EcsQuery create_query_( const String& name, void* data );
    void emit_event_( const rtti::TypeInfo* type, EcsEntity target, const void* data ) const;

    struct Impl;
    std::unique_ptr<Impl> pImpl;

    String unnamed = "Unnamed Entity";
};

} // namespace nc
