#pragma once
#include <ncore/kernel/structures.h>

#include "events.h"

namespace ncore {

enum class ButtonIndex {
    UNKNOWN = 0,
    LEFT,
    MIDDLE,
    RIGHT,
};

enum class ButtonAction {
    UNKNOWN = 0,
    PRESS,
    RELEASE
};

class NCORE_API MouseButtonEvent : public InputEvent {
    NCLASS( MouseButtonEvent, InputEvent )

public:
    MouseButtonEvent( size_t p_window_id, ButtonAction act, ButtonIndex btn, Vec2 pos ) :
        InputEvent( p_window_id ), action( act ), button( btn ), position( pos )
    {}

    EventType get_type() const override
    {
        return EventType::MOUSE_BUTTON;
    }

    ButtonAction action;
    ButtonIndex button;
    Vec2 position; // Screen position
};

class NCORE_API MouseMotionEvent : public InputEvent {
    NCLASS( MouseMotionEvent, InputEvent )

public:
    MouseMotionEvent( size_t p_window_id, Vec2 pos, Vec2 d, uint32_t state ) :
        InputEvent( p_window_id ), position( pos ), delta( d ), buttonState( state )
    {}

    EventType get_type() const override
    {
        return EventType::MOUSE_MOTION;
    }

    Vec2 position; // Current screen position
    Vec2 delta;    // Relative motion since last event
    uint32_t buttonState;
};

class NCORE_API MouseWheelEvent : public InputEvent {
    NCLASS( MouseWheelEvent, InputEvent )

public:
    MouseWheelEvent( size_t p_window_id, float x, float y ) : InputEvent( p_window_id ), scroll_x( x ), scroll_y( y ) {}

    EventType get_type() const override
    {
        return EventType::MOUSE_WHEEL;
    }

    float scroll_x;
    float scroll_y;
};

class NCORE_API KeyboardEvent : public InputEvent {
    NCLASS( KeyboardEvent, InputEvent )

public:
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

    KeyboardEvent( size_t p_window_id, ButtonAction act, Key k, bool rep ) :
        InputEvent( p_window_id ), action( act ), key( k ), repeat( rep )
    {}

    EventType get_type() const override
    {
        return EventType::KEYBOARD;
    }

    ButtonAction action;
    Key key;
    bool repeat; // Is this a key repeat?
};

class NCORE_API TextInputEvent : public InputEvent {
    NCLASS( TextInputEvent, InputEvent )

public:
    TextInputEvent( size_t p_window_id, std::string t ) : InputEvent( p_window_id ), text( std::move( t ) ) {}
    EventType get_type() const override
    {
        return EventType::TEXT_INPUT;
    }
    std::string text;
};

} // namespace ncore
