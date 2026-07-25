#pragma once

#include <cstdint>
#include <variant>

#include <ncore/core/structures.h>

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
    BKSP,
    F5,

    // Letters (missing from WASD)
    B,
    C,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    T,
    U,
    V,
    X,
    Y,
    Z,

    // Numbers
    _0,
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,

    // Function keys
    F1,
    F2,
    F3,
    F4,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    // Text navigation
    HOME,
    END,
    PAGEUP,
    PAGEDOWN,
    INSERT,
    DEL,

    // Punctuation
    PERIOD,
    COMMA,
    MINUS,
    EQUALS,
    SEMICOLON,
    QUOTE,
    BACKSLASH,
    SLASH,
    LBRACKET,
    RBRACKET,
    BACKQUOTE,

    // Misc
    CAPSLOCK,
    APPS,
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

using InputEvent = std::variant<MouseButtonEvent, MouseMotionEvent, MouseWheelEvent, KeyEvent, TextInputEvent>;

} // namespace nc
