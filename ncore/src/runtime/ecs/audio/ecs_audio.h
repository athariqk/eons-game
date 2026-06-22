#pragma once

#include <ncore/modules/ecs/ecs_module.h>

namespace ncore {

class EcsAudio : public EcsModule {
    NCLASS(EcsAudio, EcsModule)

public:
    void build(EcsWorld &world) override {}
};

} // namespace ncore
