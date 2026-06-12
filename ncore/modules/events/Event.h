#pragma once

#include <modules/events/EventType.h>
#include <modules/utils/Structures.h>

namespace ncore {

/**
 * @brief Base class for all events
 *
 * Events are lightweight data structures that represent something
 * that happened in the game (input, collision, gameplay event, etc.)
 */
struct Event {
    virtual ~Event() = default;
    virtual EventType get_type() const { return EventType::UNKNOWN; }
    bool handled = false; // prevents further processing when true
};

#define NC_REG_EVENT_TYPE(type)                                                                                        \
    static constexpr EventType static_type = type;                                                                     \
    EventType get_type() const override { return static_type; }

// -------- DERIVED TYPES --------

struct WindowEvent : public Event {
    size_t window_id;
    WindowEvent(size_t id) : window_id(id) {}
};

struct InputEvent : public WindowEvent {
    InputEvent(size_t p_window_id) : WindowEvent(p_window_id) {}
};

struct QuitEvent : public WindowEvent {
    NC_REG_EVENT_TYPE(EventType::USER_QUIT)
    QuitEvent(size_t p_window_id) : WindowEvent(p_window_id) {}
};

struct WindowResizeEvent : public WindowEvent {
    NC_REG_EVENT_TYPE(EventType::WINDOW_RESIZE)
    int width;
    int height;
    WindowResizeEvent(size_t p_window_id, int w, int h) : WindowEvent(p_window_id), width(w), height(h) {}
};

struct WindowFocusEvent : public WindowEvent {
    NC_REG_EVENT_TYPE(EventType::WINDOW_FOCUS)
    bool focused;
    WindowFocusEvent(size_t p_window_id, bool p_is_focused) : WindowEvent(p_window_id), focused(p_is_focused) {}
};

struct WindowMouseEnterEvent : public WindowEvent {
    NC_REG_EVENT_TYPE(EventType::WINDOW_MOUSE_ENTER)
    WindowMouseEnterEvent(size_t p_window_id) : WindowEvent(p_window_id) {}
};

struct WindowMouseLeaveEvent : public WindowEvent {
    NC_REG_EVENT_TYPE(EventType::WINDOW_MOUSE_LEAVE)
    WindowMouseLeaveEvent(size_t p_window_id) : WindowEvent(p_window_id) {}
};

} // namespace ncore
