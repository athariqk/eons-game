#include "ecs_graphics.h"

#include <ncore/modules/ecs/ecs_system.h>
#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/modules/gui/gui_service.h>
#include <ncore/modules/service_locator.h>
#include <ncore/modules/video/render_service.h>

namespace ncore {

void EcsGraphicsFeature::build( EcsWorld& world )
{
    world.create_system( "EcsGraphicsFeature::Render::Prepare" )
        .in( EcsSystemPhase::PreUpdate )
        .iter( []( EcsIter& iter ) {
            auto& services = ServiceLocator::get_instance();
            services.resolve<IRenderService>()->new_frame();
            services.resolve<IImGuiService>()->begin_frame();
        } );

    world.create_system( "EcsGraphicsFeature::Render::Debug" ).in( EcsSystemPhase::Update ).iter( []( EcsIter& iter ) {
        ImGui::Begin( "Render Debug" );
        ImGui::Text( "This is a debug window for rendering info." );
        ImGui::End();
    } );

    world.create_system( "EcsGraphicsFeature::Render::Present" )
        .in( EcsSystemPhase::PostUpdate )
        .iter( []( EcsIter& iter ) {
            auto& services = ServiceLocator::get_instance();
            services.resolve<IImGuiService>()->render_frame();
            services.resolve<IRenderService>()->present_frame();
        } );
}

} // namespace ncore
