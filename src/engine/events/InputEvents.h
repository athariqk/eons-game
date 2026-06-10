#pragma once

#include <EventBus.h>
#include <Vector2D.h>

#include <SDL3/SDL_events.h>
#include <cstdint>

namespace Aeon {

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
    Vector2D position; // Screen position

    MouseButtonEvent(ButtonAction act, ButtonIndex btn, Vector2D pos) : action(act), button(btn), position(pos) {}
};

struct MouseMotionEvent : public Event {
    Vector2D position; // Current screen position
    Vector2D delta; // Relative motion since last event
    uint32_t buttonState;

    MouseMotionEvent(Vector2D pos, Vector2D d, uint32_t state) : position(pos), delta(d), buttonState(state) {}

    std::string ToString();
};

struct MouseWheelEvent : public Event {
    float scrollX;
    float scrollY;

    MouseWheelEvent(float x, float y) : scrollX(x), scrollY(y) {}
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

} // namespace Aeon
