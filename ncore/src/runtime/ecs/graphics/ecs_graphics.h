#pragma once

#include <ncore/modules/ecs/ecs_module.h>

namespace ncore {

class EcsGraphics : public EcsModule {
    NCLASS(EcsGraphics, EcsModule)

public:
    void build(EcsWorld &world) override;
};

} // namespace ncore
