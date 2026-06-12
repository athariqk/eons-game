#include "ResourceManager.h"

#include <imgui.h>

namespace ncore {

void ResourceManager::unload_all() {
    pools.clear();
}

void ResourceManager::on_gui_render() {
    ImGui::Begin("Resource Manager");

    ImGui::Text("Registered pools: %zu", pools.size());

    for (auto &[type, pool_ptr] : pools) {
        (void)type;
        (void)pool_ptr;
    }

    ImGui::End();
}

} // namespace ncore
