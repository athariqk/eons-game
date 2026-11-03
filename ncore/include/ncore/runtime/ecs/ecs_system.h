#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/ecs/ecs_query.h>
#include <ncore/types/typedb.h>

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

// Sort key attached to system entities; finalize_ordering() reads it.
NC_COMPONENT( SystemOrder )
{
    REFLECT int32_t value = 0;
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
    EcsSystemBuilder( EcsWorld& p_world NC_LIFETIME_BOUND, const String& p_name );
    ~EcsSystemBuilder();

    EcsSystemBuilder( const EcsSystemBuilder& )            = delete;
    EcsSystemBuilder& operator=( const EcsSystemBuilder& ) = delete;

    template<typename... Comps>
    EcsSystemBuilder& with() NC_LIFETIME_BOUND
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

    template<typename... Comps>
    EcsSystemBuilder& without() NC_LIFETIME_BOUND
    {
        qb_.without<Comps...>();
        return *this;
    }

    EcsSystemBuilder& up() NC_LIFETIME_BOUND
    {
        qb_.up();
        return *this;
    }

    EcsSystemBuilder& self() NC_LIFETIME_BOUND
    {
        qb_.self();
        return *this;
    }

    EcsSystemBuilder& skip_self() NC_LIFETIME_BOUND
    {
        qb_.skip_self();
        return *this;
    }

    EcsSystemBuilder& cascade() NC_LIFETIME_BOUND
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

    EcsSystemBuilder& all() NC_LIFETIME_BOUND
    {
        qb_.all();
        return *this;
    }

    EcsSystemBuilder& all_read() NC_LIFETIME_BOUND
    {
        qb_.all_read();
        return *this;
    }

    EcsSystemBuilder& src( EcsEntity id ) NC_LIFETIME_BOUND
    {
        qb_.src( id );
        return *this;
    }

    EcsSystemBuilder& in( EcsSystemPhase phase ) NC_LIFETIME_BOUND;

    EcsSystemBuilder& order( int32_t priority ) NC_LIFETIME_BOUND;

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
    EcsObserverBuilder( EcsWorld& world NC_LIFETIME_BOUND, const String& name );
    ~EcsObserverBuilder();

    EcsObserverBuilder( const EcsObserverBuilder& )            = delete;
    EcsObserverBuilder& operator=( const EcsObserverBuilder& ) = delete;

    template<class... Comps>
    EcsObserverBuilder& on( EcsEntity p_event_id ) NC_LIFETIME_BOUND
    {
        qb_.with<Comps...>();
        events.push_back( p_event_id );
        return *this;
    }

    template<typename... Comps>
    EcsObserverBuilder& with() NC_LIFETIME_BOUND
    {
        qb_.with<Comps...>();
        return *this;
    }

    template<typename... Comps>
    EcsObserverBuilder& without() NC_LIFETIME_BOUND
    {
        qb_.without<Comps...>();
        return *this;
    }

    EcsObserverBuilder& event( EcsEntity p_event_id ) NC_LIFETIME_BOUND
    {
        events.push_back( p_event_id );
        return *this;
    }

    template<class T>
    EcsObserverBuilder& event() NC_LIFETIME_BOUND
    {
        auto type = TypeDB::find<T>();
        add_event_( type );
        return *this;
    }

    /**
     * @brief Traverse relationship bottom-up.
     */
    EcsObserverBuilder& up() NC_LIFETIME_BOUND
    {
        qb_.up();
        return *this;
    }

    EcsObserverBuilder& self() NC_LIFETIME_BOUND
    {
        qb_.self();
        return *this;
    }

    EcsObserverBuilder& skip_self() NC_LIFETIME_BOUND
    {
        qb_.skip_self();
        return *this;
    }

    /**
     * @brief Traverse relationship top-down.
     */
    EcsObserverBuilder& cascade() NC_LIFETIME_BOUND
    {
        qb_.cascade();
        return *this;
    }

    EcsObserverBuilder& filter() NC_LIFETIME_BOUND
    {
        qb_.filter();
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
    void add_event_( const types::TypeInfo* type );
};

} // namespace nc
