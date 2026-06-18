#pragma once

namespace ncore {

enum class EventType : size_t {
    UNKNOWN = 0,

    // General
    USER_QUIT,
    WORLD_CHANGE_REQUEST,
    WORLD_CHANGE_COMPLETE,

    // Mouse
    MOUSE_BUTTON,
    MOUSE_MOTION,
    MOUSE_WHEEL,

    // Keyboard
    KEYBOARD,

    // Window
    WINDOW_RESIZE,
    WINDOW_FOCUS,
    WINDOW_MOUSE_ENTER,
    WINDOW_MOUSE_LEAVE,

    // Text
    TEXT_INPUT,

    COUNT
};

} // namespace ncore
