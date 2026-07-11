#pragma once

#include <cstdint>
#include <variant>

#include <ncore/kernel/structures.h>

namespace nc {

enum class ButtonIndex : uint8_t {
    UNKNOWN = 0,
    LEFT,
    MIDDLE,
    RIGHT
};

enum class ButtonAction : uint8_t {
    UNKNOWN = 0,
    PRESS,
    RELEASE
};

enum class Key : uint8_t {
    UNKNOWN = 0,
    W,
    A,
    S,
    D,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    SPACE,
    ENTER,
    ESC,
    SHIFT,
    CTRL,
    ALT,
    TAB,
    BKSP
};

struct MouseButtonEvent {
    uint32_t window_id;
    ButtonAction action;
    ButtonIndex button;
    Vec2 position;
};

struct MouseMotionEvent {
    uint32_t window_id;
    Vec2 position;
    Vec2 delta;
    uint32_t button_state;
};

struct MouseWheelEvent {
    uint32_t window_id;
    float scroll_x;
    float scroll_y;
};

struct KeyEvent {
    uint32_t window_id;
    ButtonAction action;
    Key key;
    bool repeat;
};

struct TextInputEvent {
    uint32_t window_id;
    char text[32];
};

using InputEvent = std::variant<
    MouseButtonEvent,
    MouseMotionEvent,
    MouseWheelEvent,
    KeyEvent,
    TextInputEvent
>;

} // namespace nc
