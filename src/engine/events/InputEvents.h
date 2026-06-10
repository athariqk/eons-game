#pragma once

#include <EventBus.h>
#include <Vec2D.h>

#include <SDL3/SDL_events.h>
#include <cstdint>

namespace ncore {

enum class ButtonIndex {
    Unknown = 0,
    Left,
    Middle,
    Right,
};

enum class ButtonAction { Unknown = 0, Press, Release };

struct MouseButtonEvent : public Event {
    ButtonAction action;
    ButtonIndex button;
    Vec2D position; // Screen position

    MouseButtonEvent(ButtonAction act, ButtonIndex btn, Vec2D pos) : action(act), button(btn), position(pos) {}
};

struct MouseMotionEvent : public Event {
    Vec2D position; // Current screen position
    Vec2D delta; // Relative motion since last event
    uint32_t buttonState;

    MouseMotionEvent(Vec2D pos, Vec2D d, uint32_t state) : position(pos), delta(d), buttonState(state) {}

    std::string ToString();
};

struct MouseWheelEvent : public Event {
    float scroll_x;
    float scroll_y;

    MouseWheelEvent(float x, float y) : scroll_x(x), scroll_y(y) {}
};

struct KeyboardEvent : public Event {
    enum class Key {
        Unknown = 0,
        W,
        A,
        S,
        D,
        Up,
        Down,
        Left,
        Right,
        Space,
        Enter,
        Escape,
        Shift,
        Ctrl,
        Alt,
        Tab,
        Backspace,
    };

    ButtonAction action;
    Key key;
    bool repeat; // Is this a key repeat?

    KeyboardEvent(ButtonAction act, Key k, bool rep) : action(act), key(k), repeat(rep) {}

    std::string ToString();
};

struct WindowResizeEvent : public Event {
    int width;
    int height;

    WindowResizeEvent(int w, int h) : width(w), height(h) {}
};

struct WindowCloseEvent : public Event {
    WindowCloseEvent() = default;
};

struct WindowFocusEvent : public Event {
    bool focused;

    WindowFocusEvent(bool f) : focused(f) {}
};

struct TextInputEvent : public Event {
    std::string text;
    TextInputEvent(std::string t) : text(std::move(t)) {}
};

struct WindowMouseEnterEvent : public Event {
    WindowMouseEnterEvent() = default;
};

struct WindowMouseLeaveEvent : public Event {
    WindowMouseLeaveEvent() = default;
};

} // namespace ncore
