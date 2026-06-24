#include "ecs_debug.h"

#include <imgui.h>

namespace ncore {

void EcsDebugFeature::build(EcsWorld &world) {
    world.create_system("EcsDebugFeature::Stats::Update").in(EcsSystemPhase::Update).iter([](EcsIter &iter) {
        ImGui::Begin("ECS Debug");
        ImGui::Text("Entity count:\n Total: %zu\n Alive: %zu", iter.world().get_entity_count(),
                    iter.world().get_entity_count(true));
        ImGui::End();
    });
}

} // namespace ncore
