#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"

#include <SDL3/SDL_events.h>

#include <ncore/services/input/input_event.h>
#include <ncore/services/video/window/window_types.h>

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
            case SDL_SCANCODE_F5:
                return Key::F5;

            // Letters
            case SDL_SCANCODE_B:
                return Key::B;
            case SDL_SCANCODE_C:
                return Key::C;
            case SDL_SCANCODE_E:
                return Key::E;
            case SDL_SCANCODE_F:
                return Key::F;
            case SDL_SCANCODE_G:
                return Key::G;
            case SDL_SCANCODE_H:
                return Key::H;
            case SDL_SCANCODE_I:
                return Key::I;
            case SDL_SCANCODE_J:
                return Key::J;
            case SDL_SCANCODE_K:
                return Key::K;
            case SDL_SCANCODE_L:
                return Key::L;
            case SDL_SCANCODE_M:
                return Key::M;
            case SDL_SCANCODE_N:
                return Key::N;
            case SDL_SCANCODE_O:
                return Key::O;
            case SDL_SCANCODE_P:
                return Key::P;
            case SDL_SCANCODE_Q:
                return Key::Q;
            case SDL_SCANCODE_R:
                return Key::R;
            case SDL_SCANCODE_T:
                return Key::T;
            case SDL_SCANCODE_U:
                return Key::U;
            case SDL_SCANCODE_V:
                return Key::V;
            case SDL_SCANCODE_X:
                return Key::X;
            case SDL_SCANCODE_Y:
                return Key::Y;
            case SDL_SCANCODE_Z:
                return Key::Z;

            // Numbers
            case SDL_SCANCODE_0:
                return Key::_0;
            case SDL_SCANCODE_1:
                return Key::_1;
            case SDL_SCANCODE_2:
                return Key::_2;
            case SDL_SCANCODE_3:
                return Key::_3;
            case SDL_SCANCODE_4:
                return Key::_4;
            case SDL_SCANCODE_5:
                return Key::_5;
            case SDL_SCANCODE_6:
                return Key::_6;
            case SDL_SCANCODE_7:
                return Key::_7;
            case SDL_SCANCODE_8:
                return Key::_8;
            case SDL_SCANCODE_9:
                return Key::_9;

            // Function keys
            case SDL_SCANCODE_F1:
                return Key::F1;
            case SDL_SCANCODE_F2:
                return Key::F2;
            case SDL_SCANCODE_F3:
                return Key::F3;
            case SDL_SCANCODE_F4:
                return Key::F4;
            case SDL_SCANCODE_F6:
                return Key::F6;
            case SDL_SCANCODE_F7:
                return Key::F7;
            case SDL_SCANCODE_F8:
                return Key::F8;
            case SDL_SCANCODE_F9:
                return Key::F9;
            case SDL_SCANCODE_F10:
                return Key::F10;
            case SDL_SCANCODE_F11:
                return Key::F11;
            case SDL_SCANCODE_F12:
                return Key::F12;

            // Text navigation
            case SDL_SCANCODE_HOME:
                return Key::HOME;
            case SDL_SCANCODE_END:
                return Key::END;
            case SDL_SCANCODE_PAGEUP:
                return Key::PAGEUP;
            case SDL_SCANCODE_PAGEDOWN:
                return Key::PAGEDOWN;
            case SDL_SCANCODE_INSERT:
                return Key::INSERT;
            case SDL_SCANCODE_DELETE:
                return Key::DEL;

            // Punctuation
            case SDL_SCANCODE_PERIOD:
                return Key::PERIOD;
            case SDL_SCANCODE_COMMA:
                return Key::COMMA;
            case SDL_SCANCODE_MINUS:
                return Key::MINUS;
            case SDL_SCANCODE_EQUALS:
                return Key::EQUALS;
            case SDL_SCANCODE_SEMICOLON:
                return Key::SEMICOLON;
            case SDL_SCANCODE_APOSTROPHE:
                return Key::QUOTE;
            case SDL_SCANCODE_BACKSLASH:
                return Key::BACKSLASH;
            case SDL_SCANCODE_SLASH:
                return Key::SLASH;
            case SDL_SCANCODE_LEFTBRACKET:
                return Key::LBRACKET;
            case SDL_SCANCODE_RIGHTBRACKET:
                return Key::RBRACKET;
            case SDL_SCANCODE_GRAVE:
                return Key::BACKQUOTE;

            // Misc
            case SDL_SCANCODE_CAPSLOCK:
                return Key::CAPSLOCK;
            case SDL_SCANCODE_APPLICATION:
                return Key::APPS;

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
