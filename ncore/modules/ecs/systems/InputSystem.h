#pragma once

#include <unordered_set>

#include <modules/ecs/System.h>
#include <modules/events/InputEvent.h>

namespace ncore {

/**
 * @brief Input system that tracks keyboard and mouse state
 *
 * This system subscribes to input events and maintains the current
 * state of all keys and mouse buttons. Other systems can query this
 * to check if a key/button is currently pressed.
 *
 * Priority: -50 (runs early, before gameplay logic)
 */
class InputSystem : public System {
public:
    InputSystem() {
        set_priority(-50); // Run early, before gameplay
    }

    bool on_init(World &world) override;
    void on_post_update(World &world, double delta) override;
    void on_gui_render(World &world) override;
    void on_shutdown(World &world) override;

    // Query methods
    bool get_is_key_pressed(KeyboardEvent::Key key) const { return m_pressedKeys.contains(key); }
    bool get_is_mouse_button_pressed(ButtonIndex button) const { return m_pressedButtons.contains(button); }
    Vec2 get_mouse_position() const { return m_mousePosition; }
    Vec2 get_last_mouse_delta() const { return m_lastMouseDelta; }
    Vec2 get_last_mouse_wheel_delta() const { return m_lastMouseWheelDelta; }

private:
    std::unordered_set<KeyboardEvent::Key> m_pressedKeys;
    std::unordered_set<ButtonIndex> m_pressedButtons;
    Vec2 m_mousePosition{0, 0};
    Vec2 m_lastMouseDelta{0, 0};
    Vec2 m_lastMouseWheelDelta{0, 0};

    size_t m_keyDownSub = 0;
    size_t m_mouseButtonSub = 0;
    size_t m_mouseMotionSub = 0;
    size_t m_mouseWheelSub = 0;
};

} // namespace ncore
