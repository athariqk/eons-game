#pragma once

#include <SDL3/SDL_events.h>
#include <cstdint>
#include <string>

#include <Vector2D.h>

class BaseEvent {
public:
    virtual ~BaseEvent() = 0;

    virtual std::string ToString() = 0;

    SDL_Event m_handle; // HACK

private:
};

class MouseEvent : public BaseEvent {
public:
    bool m_isPanning = false;
    Vector2D m_relativeMotion;
    uint8_t m_mouseButton = 0;

    std::string ToString();
};

class KeyEvent : public BaseEvent {
public:
    enum class Type { KeyDown, KeyUp };

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
        // Add more as needed
    };

    Type m_type;
    Key m_key;
    bool m_repeat; // Is this a key repeat event

    std::string ToString();
};
