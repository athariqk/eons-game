#pragma once

#include <ncore/modules/ecs/ecs_module.h>
#include <ncore/modules/ecs/ecs_world.h>

namespace ncore {

/**
 * @brief EcsRuntimeFeature is a default feature bundle that
 * provides core engine functionalities for the ECS runtime.
 * This contains for example: rendering, physics, etc.
 *
 * You may also bypass this altogether and just load whichever
 * features you need via Scene.get_ecs().load_feature().
 */
class EcsRuntimeFeature : public EcsFeature {
    NCLASS(EcsRuntimeFeature, EcsFeature)

public:
    EcsRuntimeFeature() {}
    void build(EcsWorld& world) override;
};

} // namespace ncore
