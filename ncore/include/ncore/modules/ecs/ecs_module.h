#pragma once

#include <ncore/kernel/object.h>

namespace ncore {

class EcsWorld;

/**
 * @brief TODO: I wonder if we can make this just be a serialized composition of EcsWorld...
 * Imagine Godot's scene or Unity's prefabs
 */
class EcsModule : public NObject {
    NCLASS(EcsModule, NObject)

public:
    virtual ~EcsModule() = default;
    void operator()(EcsWorld &world) { build(world); }
    virtual void build(EcsWorld &world) = 0;
};

} // namespace ncore
