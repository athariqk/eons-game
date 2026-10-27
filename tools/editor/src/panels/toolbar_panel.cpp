#include <ncore/runtime/components/material.h>
#include <ncore/runtime/ecs/ecs_world.h>

#include <panels/toolbar_panel.h>

namespace nc::editor {

ToolbarPanel::ToolbarPanel() : EditorPanel( "Toolbar", DockSlot::Top, 0, true, false ) {}

void ToolbarPanel::draw( EditorContext& ctx )
{
    // The toolbar must stay pinned: no tab bar, no docking, no close button.
    ImGuiWindowClass window_class;
    window_class.DockingAllowUnclassed = true;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_HiddenTabBar;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingSplit;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverMe;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverOther;
    ImGui::SetNextWindowClass( &window_class );

    constexpr ImGuiWindowFlags kFlags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    if (!ImGui::Begin( title.c_str(), nullptr, kFlags )) {
        ImGui::End();
        return;
    }

    if (ImGui::Button( "Run" )) {
        /* TODO */
    }

    ImGui::SameLine();
    ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
    ImGui::SameLine();
    ImGui::Checkbox( "World", &ctx.global_xform_gizmo );

    struct GizmoMode {
        const char* label;
        ImGuizmo::OPERATION Operation;
    };
    static constexpr GizmoMode kModes[] = {
        { "Universal", ImGuizmo::OPERATION::UNIVERSAL },
        { "Translate", ImGuizmo::OPERATION::TRANSLATE },
        { "Rotate", ImGuizmo::OPERATION::ROTATE },
        { "Scale", ImGuizmo::OPERATION::SCALE },
    };

    for (const auto& mode : kModes) {
        ImGui::SameLine();
        const int value = static_cast<int>( mode.Operation );
        if (ImGui::RadioButton( mode.label, ctx.xform_gizmo_operation == value ))
            ctx.xform_gizmo_operation = value;
    }

    ImGui::SameLine();
    ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
    ImGui::SameLine();
    if (ImGui::Checkbox( "Wireframe", &ctx.draw_wireframe ) && ctx.ecs_world) {
        auto query = ctx.ecs_world->query( "MaterialComponentOwners" ).with<MaterialComponent>().build();
        for (auto entity : query.entities()) {
            auto* material      = entity.get_component<MaterialComponent>();
            material->draw_mode = ctx.draw_wireframe ? gfx::FillMode::WIREFRAME : gfx::FillMode::SOLID;
            entity.notify_changed<MaterialComponent>();
        }
    }

    // Plugin-contributed toolbar buttons (EditorCommand with an empty menu).
    if (ctx.commands) {
        for (const auto& command : *ctx.commands) {
            if (!command.menu.empty() || !command.invoke)
                continue;
            ImGui::SameLine();
            if (ImGui::Button( command.label.c_str() ))
                command.invoke();
        }
    }

    ImGui::End();
}

} // namespace nc::editor
