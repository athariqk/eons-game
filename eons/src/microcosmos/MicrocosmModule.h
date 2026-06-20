#pragma once

#include <ncore/runtime/ecs/ecs_module.h>

class MicrocosmModule : public ncore::EcsModule {
    NCLASS(MicrocosmModule, ncore::EcsModule)

public:
    void build(ncore::EcsWorld &world) override;
};
