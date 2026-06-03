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
 *
 * Usage:
 *   auto *input = world.GetSystem<InputSystem>();
 *   if (input->IsKeyPressed(KeyboardEvent::Key::W)) {
 *       // Move forward
 *   }
 */
class InputSystem : public System {
public:
    InputSystem() {
        SetPriority(-50); // Run early, before gameplay
    }

    bool OnInit(World &world) override;

    void OnShutdown(World &world) override;

    // Query methods
    bool IsKeyPressed(KeyboardEvent::Key key) const { return m_pressedKeys.contains(key); }

    bool IsMouseButtonPressed(uint8_t button) const { return m_pressedButtons.contains(button); }

    Vector2D GetMousePosition() const { return m_mousePosition; }

    Vector2D GetMouseDelta() const { return m_mouseDelta; }

private:
    std::unordered_set<KeyboardEvent::Key> m_pressedKeys;
    std::unordered_set<uint8_t> m_pressedButtons;
    Vector2D m_mousePosition{0, 0};
    Vector2D m_mouseDelta{0, 0};

    size_t m_keyDownSub = 0;
    size_t m_mouseButtonSub = 0;
    size_t m_mouseMotionSub = 0;
};

} // namespace Aeon
