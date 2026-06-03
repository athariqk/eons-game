#include "InputSystem.h"

#include <World.h>

namespace Aeon {

bool InputSystem::OnInit(World &world) {
    auto &eventBus = world.GetMainLoop().GetEventBus();

    // Subscribe to keyboard events
    m_keyDownSub = eventBus.Subscribe<KeyboardEvent>([this](const KeyboardEvent &e) {
        if (e.action == KeyboardEvent::Action::Press) {
            m_pressedKeys.insert(e.key);
        } else {
            m_pressedKeys.erase(e.key);
        }
    });

    // Subscribe to mouse button events
    m_mouseButtonSub = eventBus.Subscribe<MouseButtonEvent>([this](const MouseButtonEvent &e) {
        if (e.action == MouseButtonEvent::Action::Press) {
            m_pressedButtons.insert(e.button);
        } else {
            m_pressedButtons.erase(e.button);
        }
    });

    // Subscribe to mouse motion
    m_mouseMotionSub = eventBus.Subscribe<MouseMotionEvent>([this](const MouseMotionEvent &e) {
        m_mousePosition = e.position;
        m_mouseDelta = e.delta;
    });

	return true;
}

void InputSystem::OnShutdown(World &world) {
    auto &eventBus = world.GetMainLoop().GetEventBus();
    eventBus.Unsubscribe(m_keyDownSub);
    eventBus.Unsubscribe(m_mouseButtonSub);
    eventBus.Unsubscribe(m_mouseMotionSub);
}

} // namespace Aeon
