#pragma once

#include <ncore/runtime/ecs_feature.h>

class ImGuiContext;

namespace nc {

class VideoModule;
class GraphicsModule;
class InputModule;

/**
 * @brief EcsGuiFeature provides functionality for GUI display/layout.
 *
 * Whether it is immediate-mode or retained-mode is an implementation detail.
 */
class EcsGuiFeature : public EcsFeature {
    NCLASS( EcsGuiFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override;
};

} // namespace nc
