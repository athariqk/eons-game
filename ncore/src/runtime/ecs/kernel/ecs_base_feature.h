#pragma once

#include <ncore/modules/ecs/ecs_module.h>

namespace ncore {

class EcsBaseFeature : public EcsFeature {
    NCLASS(EcsBaseFeature, EcsFeature)

public:
    void build(EcsWorld &world) override;
};

} // namespace ncore
