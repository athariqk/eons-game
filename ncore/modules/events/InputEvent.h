#pragma once

#include "Event.h"

namespace ncore {

enum class ButtonIndex {
    UNKNOWN = 0,
    LEFT,
    MIDDLE,
    RIGHT,
};

enum class ButtonAction { UNKNOWN = 0, PRESS, RELEASE };

struct MouseButtonEvent : public InputEvent {
    NC_REG_EVENT_TYPE(EventType::MOUSE_BUTTON)

    ButtonAction action;
    ButtonIndex button;
    Vec2 position; // Screen position
    MouseButtonEvent(size_t p_window_id, ButtonAction act, ButtonIndex btn, Vec2 pos) :
        InputEvent(p_window_id), action(act), button(btn), position(pos) {}
};

struct MouseMotionEvent : public InputEvent {
    NC_REG_EVENT_TYPE(EventType::MOUSE_MOTION)

    Vec2 position; // Current screen position
    Vec2 delta; // Relative motion since last event
    uint32_t buttonState;
    MouseMotionEvent(size_t p_window_id, Vec2 pos, Vec2 d, uint32_t state) :
        InputEvent(p_window_id), position(pos), delta(d), buttonState(state) {}
};

struct MouseWheelEvent : public InputEvent {
    NC_REG_EVENT_TYPE(EventType::MOUSE_WHEEL)

    float scroll_x;
    float scroll_y;
    MouseWheelEvent(size_t p_window_id, float x, float y) : InputEvent(p_window_id), scroll_x(x), scroll_y(y) {}
};

struct KeyboardEvent : public InputEvent {
    NC_REG_EVENT_TYPE(EventType::KEYBOARD)

    enum class Key {
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
    };

    ButtonAction action;
    Key key;
    bool repeat; // Is this a key repeat?
    KeyboardEvent(size_t p_window_id, ButtonAction act, Key k, bool rep) :
        InputEvent(p_window_id), action(act), key(k), repeat(rep) {}
};

struct TextInputEvent : public InputEvent {
    NC_REG_EVENT_TYPE(EventType::TEXT_INPUT)

    std::string text;
    TextInputEvent(size_t p_window_id, std::string t) : InputEvent(p_window_id), text(std::move(t)) {}
};

} // namespace ncore
