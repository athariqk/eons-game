#pragma once

#include <cstdint>
#include <variant>

#include <ncore/core/vector.h>

namespace nc {

enum class ButtonIndex : uint8_t {
    UNKNOWN = 0,
    LEFT,
    MIDDLE,
    RIGHT,
    COUNT
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

    COUNT
};

static const char* key_to_string( Key key )
{
    switch (key) {
        case Key::W:
            return "W";
        case Key::A:
            return "A";
        case Key::S:
            return "S";
        case Key::D:
            return "D";
        case Key::UP:
            return "Up";
        case Key::DOWN:
            return "Down";
        case Key::LEFT:
            return "Left";
        case Key::RIGHT:
            return "Right";
        case Key::SPACE:
            return "Space";
        case Key::ENTER:
            return "Enter";
        case Key::ESC:
            return "Escape";
        case Key::SHIFT:
            return "Shift";
        case Key::CTRL:
            return "Ctrl";
        case Key::ALT:
            return "Alt";
        case Key::TAB:
            return "Tab";
        case Key::BKSP:
            return "Backspace";
        case Key::F5:
            return "F5";
        case Key::B:
            return "B";
        case Key::C:
            return "C";
        case Key::E:
            return "E";
        case Key::F:
            return "F";
        case Key::G:
            return "G";
        case Key::H:
            return "H";
        case Key::I:
            return "I";
        case Key::J:
            return "J";
        case Key::K:
            return "K";
        case Key::L:
            return "L";
        case Key::M:
            return "M";
        case Key::N:
            return "N";
        case Key::O:
            return "O";
        case Key::P:
            return "P";
        case Key::Q:
            return "Q";
        case Key::R:
            return "R";
        case Key::T:
            return "T";
        case Key::U:
            return "U";
        case Key::V:
            return "V";
        case Key::X:
            return "X";
        case Key::Y:
            return "Y";
        case Key::Z:
            return "Z";
        case Key::_0:
            return "0";
        case Key::_1:
            return "1";
        case Key::_2:
            return "2";
        case Key::_3:
            return "3";
        case Key::_4:
            return "4";
        case Key::_5:
            return "5";
        case Key::_6:
            return "6";
        case Key::_7:
            return "7";
        case Key::_8:
            return "8";
        case Key::_9:
            return "9";
        case Key::F1:
            return "F1";
        case Key::F2:
            return "F2";
        case Key::F3:
            return "F3";
        case Key::F4:
            return "F4";
        case Key::F6:
            return "F6";
        case Key::F7:
            return "F7";
        case Key::F8:
            return "F8";
        case Key::F9:
            return "F9";
        case Key::F10:
            return "F10";
        case Key::F11:
            return "F11";
        case Key::F12:
            return "F12";
        case Key::HOME:
            return "Home";
        case Key::END:
            return "End";
        case Key::PAGEUP:
            return "PageUp";
        case Key::PAGEDOWN:
            return "PageDown";
        case Key::INSERT:
            return "Insert";
        case Key::DEL:
            return "Delete";
        case Key::PERIOD:
            return ".";
        case Key::COMMA:
            return ",";
        case Key::MINUS:
            return "-";
        case Key::EQUALS:
            return "=";
        case Key::SEMICOLON:
            return ";";
        case Key::QUOTE:
            return "'";
        case Key::BACKSLASH:
            return "\\";
        case Key::SLASH:
            return "/";
        case Key::LBRACKET:
            return "[";
        case Key::RBRACKET:
            return "]";
        case Key::BACKQUOTE:
            return "`";
        case Key::CAPSLOCK:
            return "CapsLock";
        case Key::APPS:
            return "Apps";
        case Key::UNKNOWN:
        case Key::COUNT:
            break;
    }
    return "Unknown";
}

static const char* button_action_to_string( ButtonAction action )
{
    switch (action) {
        case ButtonAction::PRESS:
            return "Press";
        case ButtonAction::RELEASE:
            return "Release";
        case ButtonAction::UNKNOWN:
            break;
    }
    return "Unknown";
}

static const char* mouse_button_to_string( ButtonIndex button )
{
    switch (button) {
        case ButtonIndex::LEFT:
            return "Left";
        case ButtonIndex::MIDDLE:
            return "Middle";
        case ButtonIndex::RIGHT:
            return "Right";
        case ButtonIndex::UNKNOWN:
        case ButtonIndex::COUNT:
            break;
    }
    return "Unknown";
}

struct MouseButtonEvent {
    uint32_t window_id  = 0;
    ButtonAction action = ButtonAction::UNKNOWN;
    ButtonIndex button  = ButtonIndex::UNKNOWN;
    Vec2 position       = Vec2();
};

struct MouseMotionEvent {
    uint32_t window_id    = 0;
    Vec2 position         = Vec2();
    Vec2 delta            = Vec2();
    uint32_t button_state = 0;
};

struct MouseWheelEvent {
    uint32_t window_id = 0;
    float scroll_x     = 0;
    float scroll_y     = 0;
};

struct KeyEvent {
    uint32_t window_id  = 0;
    ButtonAction action = ButtonAction::UNKNOWN;
    Key key             = Key::UNKNOWN;
    bool repeat         = false;
};

struct TextInputEvent {
    uint32_t window_id = 0;
    char text[32]      = {};
};

using InputEvent =
    std::variant<MouseButtonEvent, MouseMotionEvent, MouseWheelEvent, KeyEvent, TextInputEvent>;

} // namespace nc
