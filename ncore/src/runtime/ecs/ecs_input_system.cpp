#include <runtime/ecs/ecs_input_system.h>

#include <imgui.h>

#include <ncore/modules/events/event_bus.h>
#include <ncore/modules/events/input_event.h>
#include <ncore/runtime/ecs_world.h>

namespace ncore {

void EcsInputSystem::on_init(EcsWorld &world) {
    m_keyDownSub = world.get_event_bus().subscribe<KeyboardEvent>([this](KeyboardEvent &e) {
        if (e.action == ButtonAction::PRESS)
            m_pressedKeys.insert(e.key);
        else
            m_pressedKeys.erase(e.key);
    });

    m_mouseButtonSub = world.get_event_bus().subscribe<MouseButtonEvent>([this](MouseButtonEvent &e) {
        if (e.action == ButtonAction::PRESS)
            m_pressedButtons.insert(e.button);
        else
            m_pressedButtons.erase(e.button);
    });

    m_mouseMotionSub = world.get_event_bus().subscribe<MouseMotionEvent>([this](MouseMotionEvent &e) {
        m_mousePosition = e.position;
        m_lastMouseDelta = e.delta;
    });

    m_mouseWheelSub = world.get_event_bus().subscribe<MouseWheelEvent>(
        [this](MouseWheelEvent &e) { m_lastMouseWheelDelta = Vec2(e.scroll_x, e.scroll_y); });
}

void EcsInputSystem::on_post_update(EcsWorld &world, double delta) {
    m_lastMouseDelta.zero();
    m_lastMouseWheelDelta.zero();
}

void EcsInputSystem::on_gui_render(EcsWorld &world) {
    ImGui::Begin("Input");

    ImGui::Text("Mouse (%.0f, %.0f)  Delta (%.1f, %.1f)", m_mousePosition.x, m_mousePosition.y, m_lastMouseDelta.x,
                m_lastMouseDelta.y);
    ImGui::Text("Wheel (%.1f, %.1f)", m_lastMouseWheelDelta.x, m_lastMouseWheelDelta.y);

    ImGui::SeparatorText("Keys");

    static constexpr const char *keyNames[] = {
        "Unknown", "W",     "A",      "S",     "D",    "Up",  "Down", "Left",      "Right",
        "Space",   "Enter", "Escape", "Shift", "Ctrl", "Alt", "Tab",  "Backspace",
    };

    int col = 0;
    for (int k = 1; k < static_cast<int>(KeyboardEvent::Key::BKSP) + 1; k++) {
        auto key = static_cast<KeyboardEvent::Key>(k);
        bool pressed = m_pressedKeys.contains(key);
        if (pressed)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        else
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        ImGui::Button(keyNames[k], ImVec2(56, 0));
        ImGui::PopStyleColor();
        if (++col < 5)
            ImGui::SameLine();
        else
            col = 0;
    }

    ImGui::SeparatorText("Mouse Buttons");

    static constexpr const char *btnNames[] = {"Unknown", "Left", "Middle", "Right"};
    for (int b = 1; b < static_cast<int>(ButtonIndex::RIGHT) + 1; b++) {
        auto btn = static_cast<ButtonIndex>(b);
        bool pressed = m_pressedButtons.contains(btn);
        if (pressed)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        else
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        ImGui::Button(btnNames[b], ImVec2(56, 0));
        ImGui::PopStyleColor();
        if (b < static_cast<int>(ButtonIndex::RIGHT))
            ImGui::SameLine();
    }

    ImGui::SeparatorText("Event Bus");

    auto subscriberInfo = world.get_event_bus().get_subscriber_debug_info();
    ImGui::Text("Queue: %zu", world.get_event_bus().get_queue_size());
    ImGui::Text("Subscribers: %zu", subscriberInfo.size());

    for (const auto &[ev_type, count]: subscriberInfo)
        ImGui::Text("ID: %d  Subs: %zu", ev_type.value, count);

    ImGui::End();
}

void EcsInputSystem::on_shutdown(EcsWorld &world) {
    world.get_event_bus().unsubscribe(m_keyDownSub);
    world.get_event_bus().unsubscribe(m_mouseButtonSub);
    world.get_event_bus().unsubscribe(m_mouseMotionSub);
    world.get_event_bus().unsubscribe(m_mouseWheelSub);
}

} // namespace ncore
