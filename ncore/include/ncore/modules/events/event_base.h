#pragma once

#include <ncore/kernel/reference.h>

#include "event_type.h"

namespace nc {

/**
 * @brief Base class for all reference-counted event
 * objects that respects the event bus pattern.
 */
class NCORE_API BaseEvent : public RefCounted {
    NCLASS( BaseEvent, RefCounted )

public:
    virtual EventType get_type() const
    {
        return EventType::UNKNOWN;
    }

    // prevents further processing when true
    mutable bool handled = false;
};

} // namespace nc
