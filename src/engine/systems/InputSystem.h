#pragma once

#include <unordered_set>

#include <InputEvents.h>
#include <System.h>

namespace Aeon {

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
        SetPriority(-50); // Run early, before gameplay
    }

    bool OnInit(World &world) override;
    void OnPostUpdate(World &world, double delta) override;
    void OnGuiRender(World &world) override;
    void OnShutdown(World &world) override;

    // Query methods
    bool IsKeyPressed(KeyboardEvent::Key key) const { return m_pressedKeys.contains(key); }
    bool IsMouseButtonPressed(ButtonIndex button) const { return m_pressedButtons.contains(button); }
    Vector2D GetMousePosition() const { return m_mousePosition; }
    Vector2D GetLastMouseDelta() const { return m_lastMouseDelta; }
	Vector2D GetLastMouseWheelDelta() const { return m_lastMouseWheelDelta; }

private:
    std::unordered_set<KeyboardEvent::Key> m_pressedKeys;
    std::unordered_set<ButtonIndex> m_pressedButtons;
    Vector2D m_mousePosition{0, 0};
    Vector2D m_lastMouseDelta{0, 0};
    Vector2D m_lastMouseWheelDelta{0, 0};

    size_t m_keyDownSub = 0;
    size_t m_mouseButtonSub = 0;
    size_t m_mouseMotionSub = 0;
    size_t m_mouseWheelSub = 0;
};

} // namespace Aeon
