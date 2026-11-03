#include <ncore/application.h>
#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/resources/material.h>

#include <panels/toolbar_panel.h>

namespace nc::editor {

ToolbarPanel::ToolbarPanel( EditorSettings& p_settings ) :
    EditorPanel( "Toolbar", DockSlot::Top, 0, true, false ), settings( p_settings )
{}

ImGuiWindowFlags ToolbarPanel::window_flags() const
{
    return ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
}

void ToolbarPanel::pre_begin( EditorFrame& )
{
    ImGuiWindowClass window_class;
    window_class.DockingAllowUnclassed = true;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_HiddenTabBar;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingSplit;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverMe;
    window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverOther;
    ImGui::SetNextWindowClass( &window_class );
}

void ToolbarPanel::draw_contents( EditorFrame& ctx )
{
    auto& ecs_world = ctx.scene->get_ecs();

    if (ImGui::Button( "Run" )) {
        /* TODO */
    }

    ImGui::SameLine();
    ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
    ImGui::SameLine();
    ImGui::Checkbox( "World", &settings.global_xform_gizmo );

    static constexpr std::pair<const char*, ImGuizmo::OPERATION> MODES[] = {
        { "Universal", ImGuizmo::OPERATION::UNIVERSAL },
        { "Translate", ImGuizmo::OPERATION::TRANSLATE },
        { "Rotate", ImGuizmo::OPERATION::ROTATE },
        { "Scale", ImGuizmo::OPERATION::SCALE },
    };

    for (const auto& mode : MODES) {
        ImGui::SameLine();
        const int value = static_cast<int>( mode.second );
        if (ImGui::RadioButton( mode.first, settings.xform_gizmo_operation == value ))
            settings.xform_gizmo_operation = value;
    }

    ImGui::SameLine();
    ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
    ImGui::SameLine();
    if (ImGui::Checkbox( "Wireframe", &settings.draw_wireframe )) {
        // Draw mode lives on the Material resource, which is shared; edit the
        // resolved instance so every entity referencing it picks the change up.
        auto query = ecs_world.query( "MaterialComponentOwners" ).with<MaterialOverrideComponent>().build();
        for (auto entity : query.entities()) {
            auto* override = entity.get_component<MaterialOverrideComponent>();
            if (!override->material)
                continue;

            override->material->set_draw_mode(
                settings.draw_wireframe ? gfx::FillMode::WIREFRAME : gfx::FillMode::SOLID
            );
            entity.notify_changed<MaterialOverrideComponent>();
        }
    }
}

} // namespace nc::editor
