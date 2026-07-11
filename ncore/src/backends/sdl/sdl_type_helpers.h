#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"

#include <SDL3/SDL_events.h>

#include <ncore/modules/input/input_event.h>
#include <ncore/modules/video/window_types.h>

namespace nc {

struct SDLTypeHelpers {
    static Key MapSDLKeyToKey( SDL_Scancode scancode )
    {
        switch (scancode) {
            case SDL_SCANCODE_W:
                return Key::W;
            case SDL_SCANCODE_A:
                return Key::A;
            case SDL_SCANCODE_S:
                return Key::S;
            case SDL_SCANCODE_D:
                return Key::D;
            case SDL_SCANCODE_UP:
                return Key::UP;
            case SDL_SCANCODE_DOWN:
                return Key::DOWN;
            case SDL_SCANCODE_LEFT:
                return Key::LEFT;
            case SDL_SCANCODE_RIGHT:
                return Key::RIGHT;
            case SDL_SCANCODE_SPACE:
                return Key::SPACE;
            case SDL_SCANCODE_RETURN:
                return Key::ENTER;
            case SDL_SCANCODE_ESCAPE:
                return Key::ESC;
            case SDL_SCANCODE_LSHIFT:
            case SDL_SCANCODE_RSHIFT:
                return Key::SHIFT;
            case SDL_SCANCODE_LCTRL:
            case SDL_SCANCODE_RCTRL:
                return Key::CTRL;
            case SDL_SCANCODE_LALT:
            case SDL_SCANCODE_RALT:
                return Key::ALT;
            case SDL_SCANCODE_TAB:
                return Key::TAB;
            case SDL_SCANCODE_BACKSPACE:
                return Key::BKSP;
            default:
                return Key::UNKNOWN;
        }
    }

    static ButtonAction MapSDLEventTypeToAction( uint32_t p_sdl_event_type )
    {
        switch (p_sdl_event_type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                return ButtonAction::PRESS;
            case SDL_EVENT_KEY_UP:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                return ButtonAction::RELEASE;
            default:
                return ButtonAction::UNKNOWN;
        }
    }

    static ButtonIndex MapSDLButtonToButtonIndex( uint8_t p_sdl_button )
    {
        switch (p_sdl_button) {
            case SDL_BUTTON_LEFT:
                return ButtonIndex::LEFT;
            case SDL_BUTTON_MIDDLE:
                return ButtonIndex::MIDDLE;
            case SDL_BUTTON_RIGHT:
                return ButtonIndex::RIGHT;
            default:
                return ButtonIndex::UNKNOWN;
        }
    }

    static SDL_Scancode KeyToScancode( Key key )
    {
        switch (key) {
            case Key::W:
                return SDL_SCANCODE_W;
            case Key::A:
                return SDL_SCANCODE_A;
            case Key::S:
                return SDL_SCANCODE_S;
            case Key::D:
                return SDL_SCANCODE_D;
            case Key::UP:
                return SDL_SCANCODE_UP;
            case Key::DOWN:
                return SDL_SCANCODE_DOWN;
            case Key::LEFT:
                return SDL_SCANCODE_LEFT;
            case Key::RIGHT:
                return SDL_SCANCODE_RIGHT;
            case Key::SPACE:
                return SDL_SCANCODE_SPACE;
            case Key::ENTER:
                return SDL_SCANCODE_RETURN;
            case Key::ESC:
                return SDL_SCANCODE_ESCAPE;
            case Key::SHIFT:
                return SDL_SCANCODE_LSHIFT;
            case Key::CTRL:
                return SDL_SCANCODE_LCTRL;
            case Key::ALT:
                return SDL_SCANCODE_LALT;
            case Key::TAB:
                return SDL_SCANCODE_TAB;
            case Key::BKSP:
                return SDL_SCANCODE_BACKSPACE;
            case Key::UNKNOWN:
            default:
                return SDL_SCANCODE_UNKNOWN;
        }
    }

    static uint8_t BtnToSDL( ButtonIndex btn )
    {
        switch (btn) {
            case ButtonIndex::LEFT:
                return SDL_BUTTON_LEFT;
            case ButtonIndex::MIDDLE:
                return SDL_BUTTON_MIDDLE;
            case ButtonIndex::RIGHT:
                return SDL_BUTTON_RIGHT;
            case ButtonIndex::UNKNOWN:
            default:
                return 0;
        }
    }

    static CursorType from_sdl_sys_cursor( SDL_SystemCursor sdl_cursor )
    {
        switch (sdl_cursor) {
            case SDL_SYSTEM_CURSOR_DEFAULT:
                return CursorType::DEFAULT;
            case SDL_SYSTEM_CURSOR_TEXT:
                return CursorType::TEXT;
            case SDL_SYSTEM_CURSOR_CROSSHAIR:
                return CursorType::CROSSHAIR;
            case SDL_SYSTEM_CURSOR_POINTER:
                return CursorType::POINTER;
            case SDL_SYSTEM_CURSOR_EW_RESIZE:
                return CursorType::RESIZE_EW;
            case SDL_SYSTEM_CURSOR_NS_RESIZE:
                return CursorType::RESIZE_NS;
            case SDL_SYSTEM_CURSOR_NWSE_RESIZE:
                return CursorType::RESIZE_NWSE;
            case SDL_SYSTEM_CURSOR_NESW_RESIZE:
                return CursorType::RESIZE_NESW;
            case SDL_SYSTEM_CURSOR_WAIT:
                return CursorType::WAIT;
            default:
                return CursorType::DEFAULT;
        }
    }

    static SDL_SystemCursor to_sdl_sys_cursor( CursorType cursor_type )
    {
        switch (cursor_type) {
            case CursorType::DEFAULT:
                return SDL_SYSTEM_CURSOR_DEFAULT;
            case CursorType::TEXT:
                return SDL_SYSTEM_CURSOR_TEXT;
            case CursorType::CROSSHAIR:
                return SDL_SYSTEM_CURSOR_CROSSHAIR;
            case CursorType::POINTER:
                return SDL_SYSTEM_CURSOR_POINTER;
            case CursorType::RESIZE_NS:
                return SDL_SYSTEM_CURSOR_NS_RESIZE;
            case CursorType::RESIZE_EW:
                return SDL_SYSTEM_CURSOR_EW_RESIZE;
            case CursorType::RESIZE_NESW:
                return SDL_SYSTEM_CURSOR_NESW_RESIZE;
            case CursorType::RESIZE_NWSE:
                return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
            case CursorType::WAIT:
                return SDL_SYSTEM_CURSOR_WAIT;
            default:
                return SDL_SYSTEM_CURSOR_DEFAULT;
        }
    }
};

} // namespace nc

#pragma GCC diagnostic pop
