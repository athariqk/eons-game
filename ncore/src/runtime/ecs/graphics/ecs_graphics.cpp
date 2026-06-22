#include "ecs_graphics.h"

#include <ncore/modules/ecs/ecs_system.h>
#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/modules/service_locator.h>
#include <ncore/modules/video/render_service.h>

#include <modules/gui/gui_service.h>

namespace ncore {

void EcsGraphics::build(EcsWorld &world) {
    world.create_system("EcsGraphics::Render::Prepare").in(EcsSystemPhase::PreUpdate).order(0).iter([](EcsIter &iter) {
        auto &services = iter.services();
        services.resolve<IRenderService>()->new_frame();
        services.resolve<IIMGuiService>()->begin_frame();
    });

    world.create_system("EcsGraphics::Render::Debug").in(EcsSystemPhase::Update).order(0).iter([](EcsIter &iter) {
        ImGui::Begin("Render Debug");
        ImGui::Text("This is a debug window for rendering info.");
        ImGui::End();
    });

    world.create_system("EcsGraphics::Render::Present").in(EcsSystemPhase::PostUpdate).order(0).iter([](EcsIter &iter) {
        auto &services = iter.services();
        services.resolve<IIMGuiService>()->render_frame();
        services.resolve<IRenderService>()->present_frame();
    });
}

} // namespace ncore
