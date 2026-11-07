#include "toolbar_panel.h"

#include <ncore/application.h>
#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/resources/material.h>

#include <editor/editor.h>

namespace sea::editor {

ToolbarPanel::ToolbarPanel( nc::Scene& p_scene, EditorSettings& p_settings ) :
    EditorPanel( "Toolbar", DockSlot::TOP, 0, true, false ), scene( p_scene ), settings( p_settings )
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

void ToolbarPanel::draw_contents( EditorFrame& )
{
    auto& ecs_world = scene.get_ecs();

    // Live editing while simulating is the default (and desired) mode.
    // This button is a visual indicator + future hook for pause/resume if a time scale is added.
    ImGui::BeginDisabled();
    ImGui::Button( "▶ Simulating" );
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ))
        ImGui::SetTooltip(
            "The world is always simulating. You can edit assets, materials, transforms etc. live.\nHot reload and "
            "inspector changes take effect immediately."
        );

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
        auto query = ecs_world.query( "MaterialComponentOwners" ).with<nc::MaterialOverrideComponent>().build();
        for (auto entity : query.entities()) {
            auto* override = entity.get_component<nc::MaterialOverrideComponent>();
            if (!override->material)
                continue;

            override->material->set_draw_mode(
                settings.draw_wireframe ? nc::gfx::FillMode::WIREFRAME : nc::gfx::FillMode::SOLID
            );
            entity.notify_changed<nc::MaterialOverrideComponent>();
        }
    }
}

} // namespace sea::editor
