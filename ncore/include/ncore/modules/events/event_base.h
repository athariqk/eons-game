#pragma once

#include <ncore/core/reference.h>

namespace nc {

/**
 * @brief Base class for all reference-counted event
 * objects that respects the event bus pattern.
 *
 * Reserved for future game-level events (animation, physics collisions, etc).
 * For OS/window/input stuff, events use the separate per-subsystem polling
 * model (std::variant).
 */
class NCAPI BaseEvent : public RefCounted {
    NCLASS( BaseEvent, RefCounted )

public:
    // prevents further processing when true
    mutable bool handled = false;
};

} // namespace nc
