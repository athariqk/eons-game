#include "InputSystem.h"

#include <World.h>
#include <imgui.h>

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
	// Flush inputs
    m_lastMouseDelta.Zero();
    m_lastMouseWheelDelta.Zero();
}

void InputSystem::OnGuiRender(World &world) {
    auto &eventBus = world.GetMainLoop().GetEventBus();
    ImGui::Begin("Input");

    ImGui::Text("Mouse (%.0f, %.0f)  Delta (%.1f, %.1f)", m_mousePosition.x, m_mousePosition.y,
                m_lastMouseDelta.x, m_lastMouseDelta.y);
    ImGui::Text("Wheel (%.1f, %.1f)", m_lastMouseWheelDelta.x, m_lastMouseWheelDelta.y);

    ImGui::SeparatorText("Keys");

    static constexpr const char *keyNames[] = {
        "Unknown", "W", "A", "S", "D", "Up", "Down", "Left", "Right",
        "Space", "Enter", "Escape", "Shift", "Ctrl", "Alt", "Tab", "Backspace",
    };

    int col = 0;
    for (int k = 1; k < static_cast<int>(KeyboardEvent::Key::Backspace) + 1; k++) {
        auto key = static_cast<KeyboardEvent::Key>(k);
        bool pressed = m_pressedKeys.contains(key);
        if (pressed) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        else         ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        ImGui::Button(keyNames[k], ImVec2(56, 0));
        ImGui::PopStyleColor();
        if (++col < 5) ImGui::SameLine();
        else           col = 0;
    }

    ImGui::SeparatorText("Mouse Buttons");

    static constexpr const char *btnNames[] = {"Unknown", "Left", "Middle", "Right"};
    for (int b = 1; b < static_cast<int>(ButtonIndex::Right) + 1; b++) {
        auto btn = static_cast<ButtonIndex>(b);
        bool pressed = m_pressedButtons.contains(btn);
        if (pressed) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        else         ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        ImGui::Button(btnNames[b], ImVec2(56, 0));
        ImGui::PopStyleColor();
        if (b < static_cast<int>(ButtonIndex::Right)) ImGui::SameLine();
    }

    ImGui::SeparatorText("Event Bus");

    auto subscriberInfo = eventBus.GetSubscriberDebugInfo();
    ImGui::Text("Queue: %zu", eventBus.GetQueueSize());
    ImGui::Text("Subscribers: %zu", subscriberInfo.size());

    for (const auto &[name, count] : subscriberInfo) {
        if (ImGui::TreeNode(name)) {
            ImGui::Text("ID: %s  Subs: %zu", name, count);
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void InputSystem::OnShutdown(World &world) {
    auto &eventBus = world.GetMainLoop().GetEventBus();
    eventBus.Unsubscribe(m_keyDownSub);
    eventBus.Unsubscribe(m_mouseButtonSub);
    eventBus.Unsubscribe(m_mouseMotionSub);
    eventBus.Unsubscribe(m_mouseWheelSub);
}

} // namespace Aeon
