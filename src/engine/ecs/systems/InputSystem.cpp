#include "InputSystem.h"

#include <World.h>
#include <imgui.h>

namespace ncore {

bool InputSystem::on_init(World &world) {
    auto &eventBus = world.get_main_loop().get_event_bus();

    m_keyDownSub = eventBus.subscribe<KeyboardEvent>([this](const KeyboardEvent &e) {
        if (e.action == ButtonAction::Press) {
            m_pressedKeys.insert(e.key);
        } else {
            m_pressedKeys.erase(e.key);
        }
    });

    m_mouseButtonSub = eventBus.subscribe<MouseButtonEvent>([this](const MouseButtonEvent &e) {
        if (e.action == ButtonAction::Press) {
            m_pressedButtons.insert(e.button);
        } else {
            m_pressedButtons.erase(e.button);
        }
    });

    m_mouseMotionSub = eventBus.subscribe<MouseMotionEvent>([this](const MouseMotionEvent &e) {
        m_mousePosition = e.position;
        m_lastMouseDelta = e.delta;
    });

    m_mouseWheelSub = eventBus.subscribe<MouseWheelEvent>(
        [this](const MouseWheelEvent &e) { m_lastMouseWheelDelta = Vec2D(e.scroll_x, e.scroll_y); });

    return true;
}

void InputSystem::on_post_update(World &world, double delta) {
	// Flush inputs
    m_lastMouseDelta.zero();
    m_lastMouseWheelDelta.zero();
}

void InputSystem::on_gui_render(World &world) {
    auto &event_bus = world.get_main_loop().get_event_bus();
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

    auto subscriberInfo = event_bus.get_subscriber_debug_info();
    ImGui::Text("Queue: %zu", event_bus.get_queue_size());
    ImGui::Text("Subscribers: %zu", subscriberInfo.size());

    for (const auto &[name, count] : subscriberInfo) {
        if (ImGui::TreeNode(name)) {
            ImGui::Text("ID: %s  Subs: %zu", name, count);
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void InputSystem::on_shutdown(World &world) {
    auto &eventBus = world.get_main_loop().get_event_bus();
    eventBus.unsubscribe(m_keyDownSub);
    eventBus.unsubscribe(m_mouseButtonSub);
    eventBus.unsubscribe(m_mouseMotionSub);
    eventBus.unsubscribe(m_mouseWheelSub);
}

} // namespace ncore
