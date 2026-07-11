#include <flecs.h>
#include <flecs/addons/system.h>

#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

#include "flecs_helpers.h"

namespace nc {

//------------------------------------------------------------------------------
// EcsSystemBuilder
//------------------------------------------------------------------------------

struct EcsSystemBuilder::Impl {
    EcsSystemPhase phase_ = EcsSystemPhase::Update;
    int32_t order_        = 0;
    bool built_           = false;
};

EcsSystemBuilder::EcsSystemBuilder( EcsWorld& world, std::string p_name ) :
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

EcsEntityId EcsSystemBuilder::run( RunCallback callback )
{
    return init_system_( EcsCallbackKind::Run, reinterpret_cast<void*>( callback ) );
}

EcsEntityId EcsSystemBuilder::each( EachCallback callback )
{
    return init_system_( EcsCallbackKind::Each, reinterpret_cast<void*>( callback ) );
}

// just so the compiler can see the defs for Impl
struct EcsQueryBuilder::Impl : public detail::FlecsQueryBuilder {};

struct SystemOrder {
    int32_t value = 0;
    NSTRUCT( SystemOrder, NC_F( SystemOrder, value ) )
};

static void handle_iter_callback( ecs_iter_t* it )
{
    auto fn = reinterpret_cast<RunCallback>( it->ctx );
    QueryContext wrap( it );
    fn( wrap );
}

static void handle_each_callback( ecs_iter_t* it )
{
    auto fn = reinterpret_cast<EachCallback>( it->ctx );
    QueryContext wrap( it );
    for (int32_t row = 0; row < wrap.count(); row++) {
        fn( wrap, wrap.entity( row ) );
    }
}

EcsEntityId EcsSystemBuilder::init_system_( EcsCallbackKind kind, void* p_callback )
{
    auto world = static_cast<ecs_world_t*>( world_.get_native_handle() );

    // pick matching callback
    ecs_iter_action_t callback = nullptr;
    if (kind == EcsCallbackKind::Run) {
        callback = handle_iter_callback;
    } else {
        callback = handle_each_callback;
    }

    // build system descriptor
    ecs_entity_desc_t sys_ent_desc{};
    sys_ent_desc.name    = name.c_str();
    ecs_entity_t sys_ent = ecs_entity_init( world, &sys_ent_desc );
    ecs_system_desc_t sdesc{};
    sdesc.entity   = sys_ent;
    sdesc.query    = qb_.pImpl->get_as_descriptor(); // copy query terms from query builder
    sdesc.phase    = detail::map_phase( pImpl->phase_ );
    sdesc.callback = callback;
    sdesc.ctx      = p_callback;

    ecs_entity_t id = ecs_system_init( world, &sdesc );
    NC_ASSERT( id != 0, "Failed to register ECS system" );

    if (pImpl->order_ != 0) {
        int32_t order_val = pImpl->order_;
        auto validate     = world_.create_entity( name ).with<SystemOrder>( order_val ).build();
        NC_ASSERT( validate == id, "EcsEntityBuilder produces non-matching entity id" );
    }

    pImpl->built_    = true;
    qb_.pImpl->built = true; // mark the query builder as built too to silence warning
    return static_cast<EcsEntityId>( id );
}

//------------------------------------------------------------------------------
// EcsObserverBuilder
//------------------------------------------------------------------------------

nc::EcsObserverBuilder::EcsObserverBuilder( EcsWorld& world, std::string p_name ) :
    world_( world ), name( std::move( p_name ) ), qb_( world, name + "_qb" )
{}

EcsObserverBuilder::~EcsObserverBuilder() {}

EcsEntityId EcsObserverBuilder::run( RunCallback callback )
{
    return init_observer_( EcsCallbackKind::Run, reinterpret_cast<void*>( callback ) );
}

EcsEntityId EcsObserverBuilder::each( EachCallback callback )
{
    return init_observer_( EcsCallbackKind::Each, reinterpret_cast<void*>( callback ) );
}

EcsEntityId EcsObserverBuilder::init_observer_( EcsCallbackKind kind, void* p_callback )
{
    NC_ASSERT( events.size() <= 8, "Number of events exceeds the maximum allowed (8)" );

    auto world = static_cast<ecs_world_t*>( world_.get_native_handle() );

    // pick matching callback
    ecs_iter_action_t callback = nullptr;
    if (kind == EcsCallbackKind::Run) {
        callback = handle_iter_callback;
    } else {
        callback = handle_each_callback;
    }

    ecs_observer_desc_t desc{};
    desc.query = qb_.pImpl->get_as_descriptor(); // copy query terms from query builder
    for (size_t i = 0; i < events.size(); i++) {
        desc.events[i] = events[i];
    }
    desc.callback = callback;
    desc.ctx      = p_callback;

    qb_.pImpl->built = true; // mark the query builder as built to silence warning

    return ecs_observer_init( world, &desc );
}

} // namespace nc
