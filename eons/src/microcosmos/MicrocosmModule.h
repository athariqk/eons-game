#pragma once

#include <ncore/modules/ecs/ecs_module.h>

class MicrocosmModule : public ncore::EcsFeature {
    NCLASS(MicrocosmModule, ncore::EcsFeature)

public:
    void build(ncore::EcsWorld& world) override;
};
