#pragma once

#include <ncore/modules/ecs/ecs_module.h>

namespace ncore {

class EcsAudioFeature : public EcsFeature {
    NCLASS(EcsAudioFeature, EcsFeature)

public:
    void build(EcsWorld &world) override {}
};

} // namespace ncore
