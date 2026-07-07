#pragma once

#include "event_base.h"

namespace nc {

class NCORE_API WindowEvent : public BaseEvent {
    NCLASS( WindowEvent, BaseEvent )

public:
    size_t window_id;
    WindowEvent( size_t index ) : window_id( index ) {}
};

// ---------------------------------------------------------------------------

class NCORE_API WindowCloseEvent : public WindowEvent {
    NCLASS( WindowCloseEvent, WindowEvent )

public:
    WindowCloseEvent( size_t p_window_id ) : WindowEvent( p_window_id ) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_CLOSE;
    }
};

class NCORE_API WindowResizeEvent : public WindowEvent {
    NCLASS( WindowResizeEvent, WindowEvent )

public:
    WindowResizeEvent( size_t p_window_id, int w, int h ) : WindowEvent( p_window_id ), width( w ), height( h ) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_RESIZE;
    }
    int width;
    int height;
};

class NCORE_API WindowFocusEvent : public WindowEvent {
    NCLASS( WindowFocusEvent, WindowEvent )

public:
    WindowFocusEvent( size_t p_window_id, bool p_is_focused ) : WindowEvent( p_window_id ), focused( p_is_focused ) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_FOCUS;
    }
    bool focused;
};

class NCORE_API WindowMouseEnterEvent : public WindowEvent {
    NCLASS( WindowMouseEnterEvent, WindowEvent )
public:
    WindowMouseEnterEvent( size_t p_window_id ) : WindowEvent( p_window_id ) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_MOUSE_ENTER;
    }
};

class NCORE_API WindowMouseLeaveEvent : public WindowEvent {
    NCLASS( WindowMouseLeaveEvent, WindowEvent )

public:
    WindowMouseLeaveEvent( size_t p_window_id ) : WindowEvent( p_window_id ) {}
    EventType get_type() const override
    {
        return EventType::WINDOW_MOUSE_LEAVE;
    }
};

} // namespace nc
