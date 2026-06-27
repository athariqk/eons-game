#pragma once

namespace ncore {

enum class EventType : size_t {
    UNKNOWN = 0,

    // General
    WINDOW_CLOSE,
    WORLD_CHANGE_REQUEST,
    WORLD_CHANGE_COMPLETE,

    // Mouse
    MOUSE_BUTTON,
    MOUSE_MOTION,
    MOUSE_WHEEL,

    // Keyboard
    KEYBOARD,

    // IWindowService
    WINDOW_RESIZE,
    WINDOW_FOCUS,
    WINDOW_MOUSE_ENTER,
    WINDOW_MOUSE_LEAVE,

    // Text
    TEXT_INPUT,

    COUNT
};

} // namespace ncore
