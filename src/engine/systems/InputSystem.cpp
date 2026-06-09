#include "InputSystem.h"

#include <World.h>

namespace Aeon {

bool InputSystem::OnInit(World &world) {
    auto &eventBus = world.GetMainLoop().GetEventBus();

    m_keyDownSub = eventBus.Subscribe<KeyboardEvent>([this](const KeyboardEvent &e) {
        if (e.action == ButtonAction::Press) {
            m_pressedKeys.insert(e.key);
        } else {
            m_pressedKeys.erase(e.key);
        }
    });

    m_mouseButtonSub = eventBus.Subscribe<MouseButtonEvent>([this](const MouseButtonEvent &e) {
        if (e.action == ButtonAction::Press) {
            m_pressedButtons.insert(e.button);
        } else {
            m_pressedButtons.erase(e.button);
        }
    });

    m_mouseMotionSub = eventBus.Subscribe<MouseMotionEvent>([this](const MouseMotionEvent &e) {
        m_mousePosition = e.position;
        m_lastMouseDelta = e.delta;
    });

    m_mouseWheelSub = eventBus.Subscribe<MouseWheelEvent>(
        [this](const MouseWheelEvent &e) { m_lastMouseWheelDelta = Vector2D(e.scrollX, e.scrollY); });

    return true;
}

void InputSystem::OnPostUpdate(World &world, double delta) {
    m_lastMouseDelta.Zero();
    m_lastMouseWheelDelta.Zero();
}

void InputSystem::OnShutdown(World &world) {
    auto &eventBus = world.GetMainLoop().GetEventBus();
    eventBus.Unsubscribe(m_keyDownSub);
    eventBus.Unsubscribe(m_mouseButtonSub);
    eventBus.Unsubscribe(m_mouseMotionSub);
    eventBus.Unsubscribe(m_mouseWheelSub);
}

} // namespace Aeon
