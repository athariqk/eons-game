#include <cstring>

#include <SDL3/SDL_events.h>
#include <backends/sdl/sdl_type_helpers.h>

#include <ncore/services/io/input_service.h>

namespace nc {

Error InputService::init( ConfFile& cfg_file )
{
    // register default actions
    for (auto action : DEFAULT_ACTION_NAMES) {
        action_register( action );
    }
    return Error::OK;
}

void InputService::shutdown() {}

void InputService::action_register( const String& name )
{
    if (action_by_name.contains( name ))
        return;

    auto rid             = actions.acquire();
    auto action          = actions.get( rid );
    action->name         = name;
    action_by_name[name] = rid;
}

void InputService::action_bind_event( const char* name, const InputEvent& event )
{
    auto action = get_action_( name );
    if (action->source_count >= MAX_BINDS)
        return;

    auto& source = action->sources[action->source_count++];
    source       = event;
}

bool InputService::action_exists( const char* name )
{
    return action_by_name.contains( name );
}

bool InputService::action_is_held( const char* name )
{
    return get_action_( name )->state.held;
}

bool InputService::action_is_pressed( const char* name )
{
    return get_action_( name )->state.pressed;
}

bool InputService::action_is_released( const char* name )
{
    return get_action_( name )->state.released;
}

float InputService::action_get_axis( const char* neg_action, const char* pos_action )
{
    ActionBinding* negative = get_action_( neg_action );
    ActionBinding* positive = get_action_( pos_action );
    return negative->state.strength - positive->state.strength;
}

Vec2f InputService::action_get_vector(
    const char* neg_action_x, const char* pos_action_x, const char* neg_action_y, const char* pos_action_y
)
{
    float x = action_get_axis( neg_action_x, pos_action_x );
    float y = action_get_axis( neg_action_y, pos_action_y );
    return Vec2f( x, y );
}

bool InputService::is_anything_held() const
{
    return is_any_held;
}

bool InputService::is_anything_pressed() const
{
    return is_any_pressed;
}

Vec2f InputService::get_mouse_position() const
{
    return mouse_pos;
}

Vec2f InputService::get_mouse_delta() const
{
    return mouse_delta;
}

Vec2f InputService::get_mouse_wheel() const
{
    return mouse_wheel;
}

bool InputService::is_key_pressed( Key key ) const
{
    return key_states[static_cast<size_t>( key )].pressed;
}

bool InputService::is_mouse_button_pressed( ButtonIndex button ) const
{
    return mb_states[static_cast<size_t>( button )].pressed;
}

void InputService::action_list( DynamicArray<StringView>& out )
{
    for (auto& action : actions) {
        out.push_back( action.name );
    }
}

void InputService::add_input_event( const InputEvent& event )
{
    event_queue.emplace( event );
}

void InputService::update()
{
    pump_events_();

    mouse_wheel.zero();
    mouse_delta.zero();

    for (auto& ks : key_states) {
        ks.pressed  = false;
        ks.released = false;
    }

    for (auto& bs : mb_states) {
        bs.pressed  = false;
        bs.released = false;
    }

    for (const auto& event : get_events()) {
        if (auto key = std::get_if<KeyEvent>( &event )) {
            auto& ks = key_states[static_cast<size_t>( key->key )];
            if (key->action == ButtonAction::PRESS) {
                ks.held = true;
                if (!key->repeat) {
                    ks.pressed = true;
                }
                ks.strength = 1.0f;
            } else if (key->action == ButtonAction::RELEASE) {
                if (ks.held) {
                    ks.released = true;
                }
                ks.held     = false;
                ks.strength = 0.0f;
            }
        } else if (auto btn = std::get_if<MouseButtonEvent>( &event )) {
            auto& bs = mb_states[static_cast<size_t>( btn->button )];
            if (btn->action == ButtonAction::PRESS) {
                bs.held     = true;
                bs.pressed  = true;
                bs.strength = 1.0f;
            } else if (btn->action == ButtonAction::RELEASE) {
                if (bs.held) {
                    bs.released = true;
                }
                bs.held     = false;
                bs.strength = 0.0f;
            }
        } else if (auto wheel = std::get_if<MouseWheelEvent>( &event )) {
            mouse_wheel.x += wheel->scroll_x;
            mouse_wheel.y += wheel->scroll_y;
        } else if (auto motion = std::get_if<MouseMotionEvent>( &event )) {
            mouse_pos = motion->position;
            mouse_delta += motion->delta;
        }
    }

    is_any_held    = false;
    is_any_pressed = false;
    for (auto& action : actions) {
        action.state = {}; // reset state
        for (int i = 0; i < action.source_count; ++i) {
            auto& source = action.sources[i];
            ButtonState state;
            if (auto kv = std::get_if<KeyEvent>( &source )) {
                state = key_states[static_cast<size_t>( kv->key )];
            }
            if (auto mv = std::get_if<MouseButtonEvent>( &source )) {
                state = mb_states[static_cast<size_t>( mv->button )];
            }
            action.state.held |= state.held;
            action.state.pressed |= state.pressed;
            action.state.released |= state.released;
            action.state.strength = state.strength;

            is_any_held |= state.held;
            is_any_pressed |= state.pressed;
        }
    }
}

void InputService::pump_events_()
{
    event_queue.reset();

    SDL_Event sdl_events[64];

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
                event_queue.emplace(
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
                event_queue.emplace( tie );
                break;
            }
            default:
                break;
        }
    }

    count = SDL_PeepEvents( sdl_events, 64, SDL_GETEVENT, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_REMOVED );
    for (int i = 0; i < count; ++i) {
        auto& e = sdl_events[i];
        switch (e.type) {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                auto action = SDLTypeHelpers::MapSDLEventTypeToAction( e.type );
                auto btn    = SDLTypeHelpers::MapSDLButtonToButtonIndex( e.button.button );
                Vec2f pos( e.button.x, e.button.y );
                event_queue.emplace(
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
                Vec2f pos( e.motion.x, e.motion.y );
                Vec2f delta( e.motion.xrel, e.motion.yrel );
                event_queue.emplace(
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
                event_queue.emplace(
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

InputService::ActionBinding* InputService::get_action_( const char* name )
{
    auto find = action_by_name.find( name );
    NC_ASSERT( find != action_by_name.end(), "Action with requested name does not exist." );
    auto action = actions.get( find->second );
    NC_VERIFY( action );
    return action;
}

} // namespace nc
