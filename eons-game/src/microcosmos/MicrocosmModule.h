#pragma once

#include <ncore/runtime/ecs_feature.h>

class MicrocosmModule : public nc::EcsFeature {
    NCLASS( MicrocosmModule, nc::EcsFeature )

public:
    void build( nc::EcsWorld& world ) override;
};
