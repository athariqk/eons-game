#include <cstring>

#include <SDL3/SDL_events.h>
#include <backends/sdl/sdl_type_helpers.h>

#include <ncore/modules/input/input_module.h>

namespace nc {

Error InputModule::init( ConfFile& cfg_file )
{
    return Error::OK;
}

void InputModule::finalize()
{
    event_queue.clear();
}

void InputModule::pump_events()
{
    event_queue.clear();

    SDL_Event sdl_events[64];

    // Keyboard range: key down/up, text input, etc.
    int count = SDL_PeepEvents( sdl_events, 64, SDL_GETEVENT, SDL_EVENT_KEY_DOWN, SDL_EVENT_SCREEN_KEYBOARD_HIDDEN );
    for (int i = 0; i < count; ++i) {
        auto& e = sdl_events[i];
        switch (e.type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                auto key = SDLTypeHelpers::MapSDLKeyToKey( e.key.scancode );
                if (key == Key::UNKNOWN) {
                    break;
                }
                auto action = SDLTypeHelpers::MapSDLEventTypeToAction( e.type );
                event_queue.push_back(
                    KeyEvent{
                        e.key.windowID,
                        action,
                        key,
                        e.key.repeat != 0,
                    }
                );
                break;
            }
            case SDL_EVENT_TEXT_INPUT: {
                TextInputEvent tie;
                tie.window_id = e.text.windowID;
                std::memset( tie.text, 0, sizeof( tie.text ) );
                std::strncpy( tie.text, e.text.text, sizeof( tie.text ) - 1 );
                event_queue.push_back( tie );
                break;
            }
            default:
                break;
        }
    }

    // Mouse range: motion, button, wheel, etc.
    count = SDL_PeepEvents( sdl_events, 64, SDL_GETEVENT, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_REMOVED );
    for (int i = 0; i < count; ++i) {
        auto& e = sdl_events[i];
        switch (e.type) {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                auto action = SDLTypeHelpers::MapSDLEventTypeToAction( e.type );
                auto btn    = SDLTypeHelpers::MapSDLButtonToButtonIndex( e.button.button );
                Vec2 pos( e.button.x, e.button.y );
                event_queue.push_back(
                    MouseButtonEvent{
                        e.button.windowID,
                        action,
                        btn,
                        pos,
                    }
                );
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                Vec2 pos( e.motion.x, e.motion.y );
                Vec2 delta( e.motion.xrel, e.motion.yrel );
                event_queue.push_back(
                    MouseMotionEvent{
                        e.motion.windowID,
                        pos,
                        delta,
                        e.motion.state,
                    }
                );
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                event_queue.push_back(
                    MouseWheelEvent{
                        e.wheel.windowID,
                        e.wheel.x,
                        e.wheel.y,
                    }
                );
                break;
            }
            default:
                break;
        }
    }
}

} // namespace nc
