#pragma once

#include <cstdint>
#include <variant>

namespace nc {

struct WindowCloseEvent {
    uint32_t window_id;
};

struct WindowResizeEvent {
    uint32_t window_id;
    int width;
    int height;
};

struct WindowFocusEvent {
    uint32_t window_id;
    bool focused;
};

struct WindowMouseEnterEvent {
    uint32_t window_id;
};

struct WindowMouseLeaveEvent {
    uint32_t window_id;
};

using WindowEvent = std::variant<
    WindowCloseEvent,
    WindowResizeEvent,
    WindowFocusEvent,
    WindowMouseEnterEvent,
    WindowMouseLeaveEvent
>;

} // namespace nc
