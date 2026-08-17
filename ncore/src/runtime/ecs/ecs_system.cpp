#include <backends/flecs/flecs_helpers.h>
#include <flecs.h>
#include <flecs/addons/system.h>

#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

namespace nc {

static void trampoline_run( ecs_iter_t* it )
{
    auto fn = reinterpret_cast<void ( * )( EcsIterState& )>( it->ctx );
    EcsIterState s( it );
    fn( s );
}

static void trampoline_each( ecs_iter_t* it )
{
    auto fn = reinterpret_cast<void ( * )( EcsIterState& )>( it->ctx );
    EcsIterState s( it );
    for (int32_t row = 0; row < s.count(); row++) {
        s.set_row( row );
        fn( s );
    }
}

//------------------------------------------------------------------------------
// EcsSystemBuilder
//------------------------------------------------------------------------------

EcsSystemBuilder::EcsSystemBuilder( EcsWorld& world, const String& p_name ) :
    world_( world ), name( std::move( p_name ) ), qb_( world, name + "_qb" )
{}

EcsSystemBuilder::~EcsSystemBuilder()
{
    if (!built_) {
        NC_LOG_WARN_C( log::ECS, "EcsSystemBuilder '{}' discarded without being built", name );
    }
}

EcsSystemBuilder& EcsSystemBuilder::in( EcsSystemPhase phase )
{
    phase_ = phase;
    return *this;
}

EcsSystemBuilder& EcsSystemBuilder::order( int32_t priority )
{
    order_ = priority;
    return *this;
}

// just so the compiler can see the defs for Impl
struct EcsQueryBuilder::Impl : public detail::FlecsQueryBuilder {};

struct SystemOrder {
    int32_t value = 0;
    NSTRUCTV( SystemOrder, NC_F( SystemOrder, value ) )
};

EcsEntity EcsSystemBuilder::run( void ( *callback )( EcsIterState& ) )
{
    return create_system_( reinterpret_cast<void*>( trampoline_run ), reinterpret_cast<void*>( callback ), nullptr );
}

EcsEntity EcsSystemBuilder::each( void ( *callback )( EcsIterState& ) )
{
    return create_system_( reinterpret_cast<void*>( trampoline_each ), reinterpret_cast<void*>( callback ), nullptr );
}

EcsEntity EcsSystemBuilder::create_system_( void* callback, void* ctx, void ( *ctx_free )( void* ) )
{
    auto world = static_cast<ecs_world_t*>( world_.get_native_handle() );

    ecs_entity_desc_t sys_ent_desc{};
    sys_ent_desc.name = name.c_str();
    ecs_system_desc_t sdesc{};
    sdesc.entity   = ecs_entity_init( world, &sys_ent_desc );
    sdesc.query    = qb_.pImpl->get_as_descriptor();
    sdesc.phase    = detail::map_phase( phase_ );
    sdesc.callback = reinterpret_cast<ecs_iter_action_t>( callback );
    sdesc.ctx      = ctx;
    sdesc.ctx_free = reinterpret_cast<ecs_ctx_free_t>( ctx_free );

    ecs_entity_t id = ecs_system_init( world, &sdesc );
    NC_ASSERT( id != 0, "Failed to register ECS system" );

    if (order_ != 0) {
        int32_t order_val = order_;
        auto comp_id      = world_.register_component_type( rtti::TypeRegistry::find<SystemOrder>() );
        ecs_set_id( world, id, comp_id, sizeof( order_val ), &order_val );
    }

    built_           = true;
    qb_.pImpl->built = true;
    return static_cast<EcsEntity>( id );
}

//------------------------------------------------------------------------------
// EcsObserverBuilder
//------------------------------------------------------------------------------

nc::EcsObserverBuilder::EcsObserverBuilder( EcsWorld& world, const String& p_name ) :
    world_( world ), name( std::move( p_name ) ), qb_( world, name + "_qb" )
{}

EcsObserverBuilder::~EcsObserverBuilder() {}

EcsEntity EcsObserverBuilder::run( void ( *callback )( EcsIterState& ) )
{
    return create_observer_( reinterpret_cast<void*>( trampoline_run ), reinterpret_cast<void*>( callback ), nullptr );
}

EcsEntity EcsObserverBuilder::each( void ( *callback )( EcsIterState& ) )
{
    return create_observer_( reinterpret_cast<void*>( trampoline_each ), reinterpret_cast<void*>( callback ), nullptr );
}

EcsEntity EcsObserverBuilder::create_observer_( void* callback, void* ctx, void ( *ctx_free )( void* ) )
{
    NC_ASSERT( events.size() <= 8, "Number of events exceeds the maximum allowed (8)" );

    auto world = static_cast<ecs_world_t*>( world_.get_native_handle() );

    ecs_observer_desc_t desc{};
    desc.query = qb_.pImpl->get_as_descriptor();
    for (size_t i = 0; i < events.size(); i++) {
        desc.events[i] = events[i];
    }
    desc.callback = reinterpret_cast<ecs_iter_action_t>( callback );
    desc.ctx      = ctx;
    desc.ctx_free = reinterpret_cast<ecs_ctx_free_t>( ctx_free );

    qb_.pImpl->built = true;

    return ecs_observer_init( world, &desc );
}

void EcsObserverBuilder::add_event_( const rtti::TypeInfo* type )
{
    events.push_back( world_.register_component_type( type ) );
}

} // namespace nc
