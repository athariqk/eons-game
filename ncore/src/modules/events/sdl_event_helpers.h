#pragma once

#include <memory>

#include <SDL3/SDL_events.h>

#include <ncore/modules/events/input_event.h>

namespace ncore {

struct SDLEventHelpers {
    static KeyboardEvent::Key MapSDLKeyToKey(SDL_Scancode scancode)
    {
        switch (scancode) {
            case SDL_SCANCODE_W:
                return KeyboardEvent::Key::W;
            case SDL_SCANCODE_A:
                return KeyboardEvent::Key::A;
            case SDL_SCANCODE_S:
                return KeyboardEvent::Key::S;
            case SDL_SCANCODE_D:
                return KeyboardEvent::Key::D;
            case SDL_SCANCODE_UP:
                return KeyboardEvent::Key::UP;
            case SDL_SCANCODE_DOWN:
                return KeyboardEvent::Key::DOWN;
            case SDL_SCANCODE_LEFT:
                return KeyboardEvent::Key::LEFT;
            case SDL_SCANCODE_RIGHT:
                return KeyboardEvent::Key::RIGHT;
            case SDL_SCANCODE_SPACE:
                return KeyboardEvent::Key::SPACE;
            case SDL_SCANCODE_RETURN:
                return KeyboardEvent::Key::ENTER;
            case SDL_SCANCODE_ESCAPE:
                return KeyboardEvent::Key::ESC;
            case SDL_SCANCODE_LSHIFT:
            case SDL_SCANCODE_RSHIFT:
                return KeyboardEvent::Key::SHIFT;
            case SDL_SCANCODE_LCTRL:
            case SDL_SCANCODE_RCTRL:
                return KeyboardEvent::Key::CTRL;
            case SDL_SCANCODE_LALT:
            case SDL_SCANCODE_RALT:
                return KeyboardEvent::Key::ALT;
            case SDL_SCANCODE_TAB:
                return KeyboardEvent::Key::TAB;
            case SDL_SCANCODE_BACKSPACE:
                return KeyboardEvent::Key::BKSP;
            default:
                return KeyboardEvent::Key::UNKNOWN;
        }
    }

    static ButtonAction MapSDLEventTypeToAction(uint32_t p_sdl_event_type)
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

    static ButtonIndex MapSDLButtonToButtonIndex(uint8_t p_sdl_button)
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

    static SDL_Scancode KeyToScancode(KeyboardEvent::Key key)
    {
        switch (key) {
            case KeyboardEvent::Key::W:
                return SDL_SCANCODE_W;
            case KeyboardEvent::Key::A:
                return SDL_SCANCODE_A;
            case KeyboardEvent::Key::S:
                return SDL_SCANCODE_S;
            case KeyboardEvent::Key::D:
                return SDL_SCANCODE_D;
            case KeyboardEvent::Key::UP:
                return SDL_SCANCODE_UP;
            case KeyboardEvent::Key::DOWN:
                return SDL_SCANCODE_DOWN;
            case KeyboardEvent::Key::LEFT:
                return SDL_SCANCODE_LEFT;
            case KeyboardEvent::Key::RIGHT:
                return SDL_SCANCODE_RIGHT;
            case KeyboardEvent::Key::SPACE:
                return SDL_SCANCODE_SPACE;
            case KeyboardEvent::Key::ENTER:
                return SDL_SCANCODE_RETURN;
            case KeyboardEvent::Key::ESC:
                return SDL_SCANCODE_ESCAPE;
            case KeyboardEvent::Key::SHIFT:
                return SDL_SCANCODE_LSHIFT;
            case KeyboardEvent::Key::CTRL:
                return SDL_SCANCODE_LCTRL;
            case KeyboardEvent::Key::ALT:
                return SDL_SCANCODE_LALT;
            case KeyboardEvent::Key::TAB:
                return SDL_SCANCODE_TAB;
            case KeyboardEvent::Key::BKSP:
                return SDL_SCANCODE_BACKSPACE;
            default:
                return SDL_SCANCODE_UNKNOWN;
        }
    }

    static uint8_t BtnToSDL(ButtonIndex btn)
    {
        switch (btn) {
            case ButtonIndex::LEFT:
                return SDL_BUTTON_LEFT;
            case ButtonIndex::MIDDLE:
                return SDL_BUTTON_MIDDLE;
            case ButtonIndex::RIGHT:
                return SDL_BUTTON_RIGHT;
            default:
                return 0;
        }
    }

    static std::unique_ptr<Event> map_from_sdl(SDL_Event& event)
    {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                // TODO: what should be the appropriate event mapping here?
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                return std::make_unique<WindowCloseEvent>(event.window.windowID);
            case SDL_EVENT_WINDOW_RESIZED:
                return std::make_unique<WindowResizeEvent>(
                    event.window.windowID, event.window.data1, event.window.data2
                );
            case SDL_EVENT_WINDOW_MOUSE_ENTER:
                return std::make_unique<WindowMouseEnterEvent>(event.window.windowID);
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                return std::make_unique<WindowMouseLeaveEvent>(event.window.windowID);
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                return std::make_unique<WindowFocusEvent>(event.window.windowID, true);
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                return std::make_unique<WindowFocusEvent>(event.window.windowID, false);
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                auto action    = MapSDLEventTypeToAction(event.type);
                auto btn       = MapSDLButtonToButtonIndex(event.button.button);
                auto mouse_pos = Vec2(event.button.x, event.button.y);
                return std::make_unique<MouseButtonEvent>(event.window.windowID, action, btn, mouse_pos);
            }
            case SDL_EVENT_MOUSE_MOTION: {
                auto mouse_pos = Vec2(event.motion.x, event.motion.y);
                auto delta     = Vec2(event.motion.xrel, event.motion.yrel);
                auto state     = event.motion.state;
                return std::make_unique<MouseMotionEvent>(event.window.windowID, mouse_pos, delta, state);
            }
            case SDL_EVENT_MOUSE_WHEEL:
                return std::make_unique<MouseWheelEvent>(event.window.windowID, event.wheel.x, event.wheel.y);
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                auto key = MapSDLKeyToKey(event.key.scancode);
                if (key != KeyboardEvent::Key::UNKNOWN) {
                    auto action = MapSDLEventTypeToAction(event.type);
                    return std::make_unique<KeyboardEvent>(event.window.windowID, action, key, event.key.repeat);
                }
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
                return std::make_unique<TextInputEvent>(event.window.windowID, std::string(event.text.text));
            default:
                break;
        }
        NC_LOG_ERROR_C(log::EVENTS, "Received unhandled SDL event type: {}", event.type);
        return std::make_unique<Event>();
    }

    static SDL_Event map_to_sdl(const WindowEvent* event)
    {
        SDL_Event sdl{};

        switch (event->get_type()) {
            case EventType::KEYBOARD: {
                auto key_ev      = static_cast<const KeyboardEvent*>(event);
                sdl.type         = key_ev->action == ButtonAction::PRESS ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
                sdl.key.scancode = KeyToScancode(key_ev->key);
                sdl.key.key      = 0;
                sdl.key.repeat   = key_ev->repeat;
                sdl.key.windowID = event->window_id;
                break;
            }

            case EventType::MOUSE_BUTTON: {
                auto mb_ev = static_cast<const MouseButtonEvent*>(event);
                sdl.type =
                    mb_ev->action == ButtonAction::PRESS ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
                sdl.button.button   = BtnToSDL(mb_ev->button);
                sdl.button.x        = mb_ev->position.x;
                sdl.button.y        = mb_ev->position.y;
                sdl.button.windowID = event->window_id;
                break;
            }

            case EventType::MOUSE_MOTION: {
                auto mm_ev          = static_cast<const MouseMotionEvent*>(event);
                sdl.type            = SDL_EVENT_MOUSE_MOTION;
                sdl.motion.x        = mm_ev->position.x;
                sdl.motion.y        = mm_ev->position.y;
                sdl.motion.xrel     = mm_ev->delta.x;
                sdl.motion.yrel     = mm_ev->delta.y;
                sdl.motion.state    = mm_ev->buttonState;
                sdl.motion.windowID = event->window_id;
                break;
            }

            case EventType::MOUSE_WHEEL: {
                auto mw_ev         = static_cast<const MouseWheelEvent*>(event);
                sdl.type           = SDL_EVENT_MOUSE_WHEEL;
                sdl.wheel.x        = mw_ev->scroll_x;
                sdl.wheel.y        = mw_ev->scroll_y;
                sdl.wheel.windowID = event->window_id;
                break;
            }

            case EventType::TEXT_INPUT: {
                auto ti_ev        = static_cast<const TextInputEvent*>(event);
                sdl.type          = SDL_EVENT_TEXT_INPUT;
                sdl.text.text     = ti_ev->text.c_str();
                sdl.text.windowID = event->window_id;
                break;
            }

            case EventType::WINDOW_RESIZE: {
                auto wr_ev          = static_cast<const WindowResizeEvent*>(event);
                sdl.type            = SDL_EVENT_WINDOW_RESIZED;
                sdl.window.data1    = wr_ev->width;
                sdl.window.data2    = wr_ev->height;
                sdl.window.windowID = event->window_id;
                break;
            }

            case EventType::WINDOW_CLOSE: {
                sdl.type            = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
                sdl.window.windowID = event->window_id;
                break;
            }

            case EventType::WINDOW_FOCUS: {
                auto wf_ev          = static_cast<const WindowFocusEvent*>(event);
                sdl.type            = wf_ev->focused ? SDL_EVENT_WINDOW_FOCUS_GAINED : SDL_EVENT_WINDOW_FOCUS_LOST;
                sdl.window.windowID = event->window_id;
                break;
            }

            case EventType::WINDOW_MOUSE_ENTER: {
                sdl.type            = SDL_EVENT_WINDOW_MOUSE_ENTER;
                sdl.window.windowID = event->window_id;
                break;
            }
            case EventType::WINDOW_MOUSE_LEAVE: {
                sdl.type            = SDL_EVENT_WINDOW_MOUSE_LEAVE;
                sdl.window.windowID = event->window_id;
                break;
            }

            case EventType::UNKNOWN: {
                sdl.type            = SDL_EVENT_FIRST;
                sdl.window.windowID = event->window_id;
                break;
            }
        }

        return sdl;
    }
};

} // namespace ncore
