#pragma once

#include <ncore/runtime/ecs/ecs_system.h>

class OrganismSystem : public ncore::EcsSystem {
public:
    OrganismSystem() { set_priority(55); }

    void on_fixed_update(ncore::EcsWorld &world, double fixedDelta) override;
};
