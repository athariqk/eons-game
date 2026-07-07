#include "ecs_base_feature.h"

#include <ncore/runtime/components/ecs_transform.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

void EcsBaseFeature::build( EcsWorld& world )
{
    world.create_system( "EcsBaseFeature::TransformPropagator" )
        .in( EcsSystemPhase::Update )
        .with<EcsTransform>()
        .each( []( EcsIter& iter, EcsEntityId eid ) {
            auto transform = iter.get_component<EcsTransform>( 0 );
            transform->position += Vec2{ 0.1f, 0.0f };
            NC_LOG_TRACE_C(
                log::ECS, "Entity: {}, Transform: {}", eid,
                rfl::Registry::to_string( transform, rfl::Registry::get_type_id<EcsTransform>() )
            );
        } );
}

} // namespace nc
