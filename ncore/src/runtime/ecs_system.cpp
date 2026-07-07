#include <flecs.h>
#include <flecs/addons/pipeline.h>
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

// just so the compiler can see the defs for Impl
struct EcsQueryBuilder::Impl : public detail::FlecsQueryBuilder {};

EcsEntityId EcsSystemBuilder::init_system_( SystemKind kind, void* p_callback )
{
    auto* world = static_cast<ecs_world_t*>( world_.get_native_handle_() );

    // pick matching callback
    ecs_iter_action_t callback = nullptr;
    if (kind == SystemKind::Iter) {
        callback = []( ecs_iter_t* it ) {
            auto fn = reinterpret_cast<IterCallback>( it->param );
            EcsIter wrap( it );
            fn( wrap );
        };
    } else {
        callback = []( ecs_iter_t* it ) {
            auto fn = reinterpret_cast<EachCallback>( it->param );
            EcsIter wrap( it );
            for (int32_t row = 0; row < wrap.count(); row++) {
                fn( wrap, static_cast<EcsEntityId>( row ) );
            }
        };
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

    // TODO: apply ordering once custom pipelines are implemented
    ( void ) pImpl->order_;

    pImpl->built_    = true;
    qb_.pImpl->built = true; // mark the query builder as built too to silence warning
    NC_LOG_TRACE_C( log::ECS, "Registered system (entity {})", id );
    return static_cast<EcsEntityId>( id );
}

EcsEntityId EcsSystemBuilder::iter( IterCallback callback )
{
    return init_system_( SystemKind::Iter, reinterpret_cast<void*>( callback ) );
}

EcsEntityId EcsSystemBuilder::each( EachCallback callback )
{
    return init_system_( SystemKind::Each, reinterpret_cast<void*>( callback ) );
}

} // namespace nc
