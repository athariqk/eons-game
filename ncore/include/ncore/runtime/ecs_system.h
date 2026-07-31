#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ncore/core/types.h>
#include <ncore/runtime/ecs_query.h>

namespace nc {

class EcsWorld;

enum class EcsSystemPhase {
    INIT,
    PRE_FRAME,
    PRE_UPDATE,
    FIXED_UPDATE,
    UPDATE,
    POST_UPDATE,
    POST_FRAME
};

enum class EcsCallbackKind {
    RUN,
    EACH
};

using RunCallback  = void ( * )( QueryContext& );
using EachCallback = void ( * )( QueryContext&, EcsEntityId );

class NCAPI EcsSystemBuilder {
public:
    EcsSystemBuilder( EcsWorld& world, std::string name );
    ~EcsSystemBuilder();

    EcsSystemBuilder( const EcsSystemBuilder& )            = delete;
    EcsSystemBuilder& operator=( const EcsSystemBuilder& ) = delete;

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

    EcsSystemBuilder& in( EcsSystemPhase phase );

    EcsSystemBuilder& order( int32_t priority );

    EcsEntityId run( RunCallback callback );

    EcsEntityId each( EachCallback callback );

private:
    EcsWorld& world_;
    std::string name;
    EcsQueryBuilder qb_;

    EcsEntityId init_system_( EcsCallbackKind kind, void* callback );

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

class NCAPI EcsObserverBuilder {
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

    template<typename... Comps>
    EcsObserverBuilder& with()
    {
        qb_.with<Comps...>();
        return *this;
    }

    EcsObserverBuilder& event( EcsEntityId evt )
    {
        events.push_back( evt );
        return *this;
    }

    template<class T>
    EcsObserverBuilder& event()
    {
        auto type = rtti::TypeRegistry::find<T>();
        add_event_( type );
        return *this;
    }

    /**
     * @brief Set up traversal on the last added term (default: ChildOf).
     */
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
    DynArray<EcsEntityId> events;
    EcsQueryBuilder qb_;
    EcsEntityId init_observer_( EcsCallbackKind kind, void* callback );
    void add_event_( const rtti::TypeInfo* type );
};

} // namespace nc
