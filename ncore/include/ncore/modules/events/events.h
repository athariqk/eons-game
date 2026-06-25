#pragma once

#include <ncore/kernel/object.h>

#include "event_type.h"

namespace ncore {

/**
 * @brief Base class for all get_events
 *
 * Events are lightweight data structures that represent something
 * that happened in the game (input, collision, gameplay event, etc.)
 */
class Event : public NObject {
    NCLASS(Event, NObject)

public:
    virtual ~Event() = default;
    virtual EventType get_type() const
    {
        return EventType::UNKNOWN;
    }
    bool handled = false; // prevents further processing when true
};

// -------- DERIVED TYPES --------

class WindowEvent : public Event {
    NCLASS(WindowEvent, Event)

public:
    size_t window_id;
    WindowEvent(size_t index) : window_id(index) {}
};

class InputEvent : public WindowEvent {
    NCLASS(InputEvent, WindowEvent)

public:
    InputEvent(size_t p_window_id) : WindowEvent(p_window_id) {}
};

class WindowCloseEvent : public WindowEvent {
    NCLASS(WindowCloseEvent, WindowEvent)

public:
    WindowCloseEvent(size_t p_window_id) : WindowEvent(p_window_id) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_CLOSE;
    }
};

class WindowResizeEvent : public WindowEvent {
    NCLASS(WindowResizeEvent, WindowEvent)

public:
    WindowResizeEvent(size_t p_window_id, int w, int h) : WindowEvent(p_window_id), width(w), height(h) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_RESIZE;
    }
    int width;
    int height;
};

class WindowFocusEvent : public WindowEvent {
    NCLASS(WindowFocusEvent, WindowEvent)

public:
    WindowFocusEvent(size_t p_window_id, bool p_is_focused) : WindowEvent(p_window_id), focused(p_is_focused) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_FOCUS;
    }
    bool focused;
};

class WindowMouseEnterEvent : public WindowEvent {
    NCLASS(WindowMouseEnterEvent, WindowEvent)
public:
    WindowMouseEnterEvent(size_t p_window_id) : WindowEvent(p_window_id) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_MOUSE_ENTER;
    }
};

class WindowMouseLeaveEvent : public WindowEvent {
    NCLASS(WindowMouseLeaveEvent, WindowEvent)

public:
    WindowMouseLeaveEvent(size_t p_window_id) : WindowEvent(p_window_id) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_MOUSE_LEAVE;
    }
};

} // namespace ncore
