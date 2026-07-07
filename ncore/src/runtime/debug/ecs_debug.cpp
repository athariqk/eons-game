#include "ecs_debug.h"

#include <imgui.h>

namespace nc {

void EcsDebugFeature::build( EcsWorld& world )
{
    world.create_system( "EcsDebugFeature::Stats::Update" ).in( EcsSystemPhase::Update ).iter( []( EcsIter& iter ) {
        ImGui::Begin( "Debug" );

        ImGui::SeparatorText( "RTTI" );
        ImGui::Text( "Hits: %d", rtti::Registry::get_rtti_hits() );

        ImGui::SeparatorText( "Rendering" );
        ImGui::Text( "Stub" );

        ImGui::SeparatorText( "ECS Debug" );
        ImGui::Text(
            "Entity count:\n Total: %zu\n Alive: %zu", iter.world().get_entity_count(),
            iter.world().get_entity_count( true )
        );

        ImGui::End();
    } );
}

} // namespace nc
