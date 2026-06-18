#pragma once

#include <unordered_set>

#include <ncore/modules/ecs/ecs_system.h>
#include <ncore/modules/events/input_event.h>

namespace ncore {

class EventBus;

class EcsInputSystem : public EcsSystem {
    NCLASS(EcsInputSystem, EcsSystem)

public:
    EcsInputSystem() { set_priority(-50); }

    void on_init(EcsWorld &world) override;
    void on_post_update(EcsWorld &world, double delta) override;
    void on_gui_render(EcsWorld &world) override;
    void on_shutdown(EcsWorld &world) override;

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
