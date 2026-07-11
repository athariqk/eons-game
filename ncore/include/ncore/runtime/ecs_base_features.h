#pragma once

#include <ncore/runtime/ecs_feature.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

struct AppDesc;

/**
 * @brief EcsBaseFeatures is the default feature bundle which
 * provides NCORE Engine functionalities for the ECS runtime.
 * This includes but not limited to: rendering, physics, and
 * others.
 *
 * You may bypass this altogether and just load whichever
 * features you need via Scene.get_ecs().load_feature().
 */
class NCORE_API EcsBaseFeatures : public EcsFeature {
    NCLASS( EcsBaseFeatures, EcsFeature )

public:
    EcsBaseFeatures( const AppDesc& p_app_desc ) : app_desc( p_app_desc ) {}
    void build( EcsWorld& world ) override;

private:
    const AppDesc& app_desc;
};

} // namespace nc
