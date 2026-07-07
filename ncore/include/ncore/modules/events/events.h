#pragma once

#include <variant>

#include "event_base.h"
#include "input_event.h"
#include "window_event.h"

namespace nc {

// Fixed-size engine event types
// UNUSED
using CoreEvent = std::variant<
    WindowCloseEvent, WindowResizeEvent, WindowFocusEvent, WindowMouseEnterEvent, WindowMouseLeaveEvent,
    MouseButtonEvent, MouseMotionEvent, MouseWheelEvent, KeyboardEvent, TextInputEvent>;

// This is for any other event types
using Event = BaseEvent;

} // namespace nc
