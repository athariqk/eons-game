#pragma once

#include "editor.h"

#include <panels/assets_panel.h>
#include <panels/inspector_panel.h>
#include <panels/logs_panel.h>
#include <panels/material_inspector.h>
#include <panels/scene_tree_panel.h>
#include <panels/toolbar_panel.h>
#include <panels/viewport_panel.h>

namespace nc {
class Node;
class RenderService;
class ResourceLoader;
} // namespace nc

namespace nc::editor {

//------------------------------------------------------------------------------
// Built-in plugins
//------------------------------------------------------------------------------

class ToolbarPlugin final : public EditorPlugin {
public:
    StringView name() const override
    {
        return "Toolbar";
    }

    void enter( EditorInterface& ed ) override
    {
        ed.add_panel( DockSlot::Top, 0, std::make_unique<ToolbarPanel>() );
    }
};

class ViewportPlugin final : public EditorPlugin {
public:
    StringView name() const override
    {
        return "Viewports";
    }

    void enter( EditorInterface& ed ) override
    {
        auto scene_view = std::make_unique<ViewportPanel>(
            "Scene View", DockSlot::Center, 0, ViewportPanel::CameraSource::EditorCamera, false
        );
        ViewportPanel* scene =
            static_cast<ViewportPanel*>( ed.add_panel( DockSlot::Center, 0, std::move( scene_view ) ) );
        scene->add_overlay( std::make_unique<GizmoOverlay>() );
        scene->add_overlay( std::make_unique<StatsOverlay>() );

        ed.add_panel(
            DockSlot::Center, 1,
            std::make_unique<ViewportPanel>(
                "Game View", DockSlot::Center, 1, ViewportPanel::CameraSource::ActiveGameCamera, true
            )
        );
    }
};

class ScenePanelPlugin final : public EditorPlugin {
public:
    StringView name() const override
    {
        return "Scene";
    }

    void enter( EditorInterface& ed ) override
    {
        ed.add_panel( DockSlot::LeftUpper, 0, std::make_unique<SceneTreePanel>() );
        ed.add_panel( DockSlot::RightUpper, 0, std::make_unique<InspectorPanel>() );
        ed.add_inspector_handler( std::make_unique<MaterialInspectorHandler>() );
    }
};

class AssetsPlugin final : public EditorPlugin {
public:
    StringView name() const override
    {
        return "Assets";
    }

    void enter( EditorInterface& ed ) override
    {
        ed.add_panel( DockSlot::Bottom, 0, std::make_unique<AssetsPanel>() );
        add_asset_property_widget( ed );
    }
};

class LogsPlugin final : public EditorPlugin {
public:
    StringView name() const override
    {
        return "Logs";
    }

    void enter( EditorInterface& ed ) override
    {
        ed.add_panel( DockSlot::Bottom, 1, std::make_unique<LogsPanel>( false ) );
    }
};

} // namespace nc::editor
