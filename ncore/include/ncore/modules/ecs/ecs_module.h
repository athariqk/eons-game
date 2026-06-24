#pragma once

#include <ncore/kernel/object.h>

namespace ncore {

class EcsWorld;

/**
 * @brief EcsFeature is a base class for defining ECS modules that can be loaded into
 * an EcsWorld. Inspired by Bevy's concept of Plugins and Flecs' Modules.
 *
 * TODO: I wonder if we can make this just be a serialized composition of EcsWorld...
 * Imagine Godot's scene or Unity's prefabs
 */
class EcsFeature : public NObject {
    NCLASS(EcsFeature, NObject)

public:
    virtual ~EcsFeature() = default;
    void operator()(EcsWorld &world) { build(world); }

    /**
     * @brief Interact with the world here.
     */
    virtual void build(EcsWorld &world) = 0;
};

} // namespace ncore
