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
    auto fn = reinterpret_cast<void ( * )( QueryContext& )>( it->ctx );
    QueryContext qctx( it );
    fn( qctx );
}

static void trampoline_each( ecs_iter_t* it )
{
    auto fn = reinterpret_cast<void ( * )( QueryContext&, EcsEntity )>( it->ctx );
    QueryContext qctx( it );
    for (int32_t row = 0; row < qctx.count(); row++) {
        qctx.set_row( row );
        fn( qctx, qctx.entity( row ) );
    }
}

//------------------------------------------------------------------------------
// EcsSystemBuilder
//------------------------------------------------------------------------------

struct EcsSystemBuilder::Impl {
    EcsSystemPhase phase_ = EcsSystemPhase::UPDATE;
    int32_t order_        = 0;
    bool built_           = false;
};

EcsSystemBuilder::EcsSystemBuilder( EcsWorld& world, const String& p_name ) :
    world_( world ), name( std::move( p_name ) ), qb_( world, name + "_qb" ), pImpl( std::make_unique<Impl>() )
{}

EcsSystemBuilder::~EcsSystemBuilder()
{
    if (!pImpl->built_) {
        NC_LOG_WARN_C( log::ECS, "EcsSystemBuilder '{}' discarded without being built", name );
    }
}

EcsSystemBuilder& EcsSystemBuilder::in( EcsSystemPhase phase )
{
    pImpl->phase_ = phase;
    return *this;
}

EcsSystemBuilder& EcsSystemBuilder::order( int32_t priority )
{
    pImpl->order_ = priority;
    return *this;
}

// just so the compiler can see the defs for Impl
struct EcsQueryBuilder::Impl : public detail::FlecsQueryBuilder {};

struct SystemOrder {
    int32_t value = 0;
    NSTRUCT( SystemOrder, NC_F( SystemOrder, value ) )
};

EcsEntity EcsSystemBuilder::run( void ( *callback )( QueryContext& ) )
{
    return create_system_( reinterpret_cast<void*>( trampoline_run ), reinterpret_cast<void*>( callback ), nullptr );
}

EcsEntity EcsSystemBuilder::each( void ( *callback )( QueryContext&, EcsEntity ) )
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
    sdesc.phase    = detail::map_phase( pImpl->phase_ );
    sdesc.callback = reinterpret_cast<ecs_iter_action_t>( callback );
    sdesc.ctx      = ctx;
    sdesc.ctx_free = reinterpret_cast<ecs_ctx_free_t>( ctx_free );

    ecs_entity_t id = ecs_system_init( world, &sdesc );
    NC_ASSERT( id != 0, "Failed to register ECS system" );

    if (pImpl->order_ != 0) {
        int32_t order_val = pImpl->order_;
        auto validate     = world_.entity( name ).with<SystemOrder>( order_val ).build();
        NC_ASSERT( validate == id, "EcsEntityBuilder produces non-matching entity id" );
    }

    pImpl->built_    = true;
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

EcsEntity EcsObserverBuilder::run( void ( *callback )( QueryContext& ) )
{
    return create_observer_( reinterpret_cast<void*>( trampoline_run ), reinterpret_cast<void*>( callback ), nullptr );
}

EcsEntity EcsObserverBuilder::each( void ( *callback )( QueryContext&, EcsEntity ) )
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
