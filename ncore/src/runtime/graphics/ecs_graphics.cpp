#include "ecs_graphics.h"

#include <ncore/modules/gui/gui_module.h>
#include <ncore/modules/module_registry.h>
#include <ncore/modules/video/graphics_module.h>
#include <ncore/runtime/ecs_system.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

void EcsGraphicsFeature::build( EcsWorld& world )
{
    world.create_system( "EcsGraphicsFeature::Render::Prepare" )
        .in( EcsSystemPhase::PreUpdate )
        .iter( []( EcsIter& iter ) {
            iter.world().get_modules().resolve<GraphicsModule>()->begin_frame();
            iter.world().get_modules().resolve<IImGuiModule>()->begin_frame();
        } );

    world.create_system( "EcsGraphicsFeature::Render::Debug" ).in( EcsSystemPhase::Update ).iter( []( EcsIter& iter ) {
        ( void ) iter;
        ImGui::Begin( "Render Debug" );
        ImGui::Text( "This is a debug window for rendering info." );
        ImGui::End();
    } );

    world.create_system( "EcsGraphicsFeature::Render::Present" )
        .in( EcsSystemPhase::PostUpdate )
        .iter( []( EcsIter& iter ) {
            iter.world().get_modules().resolve<IImGuiModule>()->render_frame();
            iter.world().get_modules().resolve<GraphicsModule>()->end_frame();
        } );
}

} // namespace nc
