#pragma once

#include <ncore/kernel/reference.h>

namespace nc {

/**
 * @brief Base class for all reference-counted event
 * objects that respects the event bus pattern.
 *
 * Reserved for future game-level events (animation, physics collisions, etc).
 * OS/window/input events use the per-subsystem polling model (std::variant).
 */
class NCORE_API BaseEvent : public RefCounted {
    NCLASS( BaseEvent, RefCounted )

public:
    // prevents further processing when true
    mutable bool handled = false;
};

} // namespace nc
