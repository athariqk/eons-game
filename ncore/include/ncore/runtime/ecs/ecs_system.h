#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_query.h>

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

//------------------------------------------------------------------------------

template<typename Fn>
struct SystemDelegate {
    Fn func;

    explicit SystemDelegate( Fn&& f ) : func( std::move( f ) ) {}

    static void invoke_run( void* raw_iter )
    {
        EcsIterState qctx( raw_iter );
        auto* self = static_cast<SystemDelegate*>( qctx.user_ctx() );
        self->func( qctx );
    }

    static void invoke_each( void* raw_iter )
    {
        EcsIterState qctx( raw_iter );
        auto* self = static_cast<SystemDelegate*>( qctx.user_ctx() );
        for (int32_t row = 0; row < qctx.count(); row++) {
            qctx.set_row( row );
            self->func( qctx );
        }
    }

    static void destroy( void* ptr )
    {
        delete static_cast<SystemDelegate*>( ptr );
    }
};

//------------------------------------------------------------------------------

class NCAPI EcsSystemBuilder {
public:
    EcsSystemBuilder( EcsWorld& world, const String& name );
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

    EcsSystemBuilder& skip_self()
    {
        qb_.skip_self();
        return *this;
    }

    EcsSystemBuilder& cascade()
    {
        qb_.cascade();
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

    EcsEntity run( void ( *callback )( EcsIterState& ) );
    EcsEntity each( void ( *callback )( EcsIterState& ) );

    template<typename Fn>
    EcsEntity run( Fn&& callback )
    {
        using Delegate = SystemDelegate<std::decay_t<Fn>>;
        auto* heap_fn  = new Delegate( std::forward<Fn>( callback ) );
        return create_system_( reinterpret_cast<void*>( &Delegate::invoke_run ), heap_fn, &Delegate::destroy );
    }

    template<typename Fn>
    EcsEntity each( Fn&& callback )
    {
        using Delegate = SystemDelegate<std::decay_t<Fn>>;
        auto* heap_fn  = new Delegate( std::forward<Fn>( callback ) );
        return create_system_( reinterpret_cast<void*>( &Delegate::invoke_each ), heap_fn, &Delegate::destroy );
    }

private:
    EcsEntity create_system_( void* callback, void* ctx, void ( *ctx_free )( void* ) );

    EcsWorld& world_;
    String name; // the system's debug name.
    EcsQueryBuilder qb_;
    EcsSystemPhase phase_ = EcsSystemPhase::UPDATE;
    int32_t order_        = 0;
    bool built_           = false;
};

//------------------------------------------------------------------------------

class NCAPI EcsObserverBuilder {
public:
    EcsObserverBuilder( EcsWorld& world, const String& name );
    ~EcsObserverBuilder();

    EcsObserverBuilder( const EcsObserverBuilder& )            = delete;
    EcsObserverBuilder& operator=( const EcsObserverBuilder& ) = delete;

    template<class... Comps>
    EcsObserverBuilder& on( EcsEntity evt )
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

    EcsObserverBuilder& event( EcsEntity evt )
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

    EcsObserverBuilder& skip_self()
    {
        qb_.skip_self();
        return *this;
    }

    EcsObserverBuilder& cascade()
    {
        qb_.cascade();
        return *this;
    }

    EcsEntity run( void ( *callback )( EcsIterState& ) );
    EcsEntity each( void ( *callback )( EcsIterState& ) );

    template<typename Fn>
    EcsEntity run( Fn&& callback )
    {
        using Delegate = SystemDelegate<std::decay_t<Fn>>;
        auto* heap_fn  = new Delegate( std::forward<Fn>( callback ) );
        return create_observer_( reinterpret_cast<void*>( &Delegate::invoke_run ), heap_fn, &Delegate::destroy );
    }

    template<typename Fn>
    EcsEntity each( Fn&& callback )
    {
        using Delegate = SystemDelegate<std::decay_t<Fn>>;
        auto* heap_fn  = new Delegate( std::forward<Fn>( callback ) );
        return create_observer_( reinterpret_cast<void*>( &Delegate::invoke_each ), heap_fn, &Delegate::destroy );
    }

private:
    EcsWorld& world_;
    String name;
    DynamicArray<EcsEntity> events;
    EcsQueryBuilder qb_;

    EcsEntity create_observer_( void* callback, void* ctx, void ( *ctx_free )( void* ) );
    void add_event_( const rtti::TypeInfo* type );
};

} // namespace nc
