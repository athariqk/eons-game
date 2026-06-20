#pragma once

#include <ncore/runtime/ecs/ecs_system.h>

class SpeciesSystem : public ncore::EcsSystem {
public:
    SpeciesSystem() { set_priority(50); }

    void on_fixed_update(ncore::EcsWorld &world, double fixed_delta) override;
};
