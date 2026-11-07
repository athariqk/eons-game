// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: editor_bootstrap.cpp
// Contains the editor bootstrap_editor / shutdown_editor implementation
// and ECS system registration for editor mode.

#include "editor_bootstrap.h"

#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/log.h>

#include "editor.h"
#include "editor_components.h"
#include "editor_factory.h"
#include "plugins/gui_plugin.h"

namespace sea::editor {

namespace {

void install_editor_systems( nc::Scene& scene )
{
    // ImGuizmo needs the ImGui context that the GUI plugin created.
    scene.get_ecs()
        .system( "EditorImGuiInit" )
        .with<EditorGuiComponent>()
        .in( nc::EcsSystemPhase::INIT )
        .order( 20 )
        .run( []( nc::EcsIterState& it ) {
            ImGuizmo::SetImGuiContext( it.get_component<EditorGuiComponent>()->imgui_ctx );
        } );

    scene.get_ecs()
        .system( "EditorImGuizmoBeginFrame" )
        .with<EditorGuiComponent>()
        .in( nc::EcsSystemPhase::PRE_UPDATE )
        .order( -900 )
        .run( []( nc::EcsIterState& ) {
            ImGuizmo::SetOrthographic( false );
            ImGuizmo::BeginFrame();
        } );

    // Must run before every other PRE_UPDATE system that submits a window: the
    // dockspace node has to exist before the first Begin() of the frame.
    scene.get_ecs()
        .system( "EditorDockspace" )
        .with<EditorPlugin>()
        .in( nc::EcsSystemPhase::PRE_UPDATE )
        .order( -950 )
        .run( []( nc::EcsIterState& it ) { it.get_component<EditorPlugin>()->editor->draw_dockspace(); } );

    // After NewFrame (-1000) and ImGuizmo::BeginFrame (-900), before the
    // engine's camera update (0) so render targets are bound in time.
    scene.get_ecs()
        .system( "EditorUi" )
        .with<EditorPlugin>()
        .in( nc::EcsSystemPhase::UPDATE )
        .order( -30 )
        .run( []( nc::EcsIterState& it ) {
            auto edp = it.get_component<EditorPlugin>();
            edp->editor->begin_frame();
            edp->editor->draw();
        } );

    // ImGui resolves hover/capture state by the end of the frame.
    scene.get_ecs()
        .system( "EditorUiCapture" )
        .with<EditorPlugin>()
        .in( nc::EcsSystemPhase::POST_UPDATE )
        .run( []( nc::EcsIterState& it ) { it.get_component<EditorPlugin>()->editor->publish_ui_capture(); } );
}

} // namespace

void bootstrap_editor( nc::Scene& p_scene )
{
    auto& world = p_scene.get_ecs();
    if (world.get_singleton<EditorPlugin>()) {
        NC_LOG_WARN( "Editor already bootstrapped; skipping." );
        return;
    }

    register_editor_component_types( p_scene );
    register_gui_plugin( p_scene );

    auto editor = create_editor( p_scene );
    if (!editor) {
        NC_LOG_ERROR( "Editor requires a RenderService." );
        return;
    }

    world.add_singleton<EditorPlugin>();
    world.get_singleton<EditorPlugin>()->editor = std::move( editor );

    install_editor_systems( p_scene );

    p_scene.get_ecs().finalize_ordering();
}

void shutdown_editor( nc::Scene& p_scene )
{
    auto& world = p_scene.get_ecs();
    auto edp    = world.get_singleton_const<EditorPlugin>();
    if (!edp) {
        NC_LOG_WARN( "Editor not present for shutdown." );
        return;
    }

    // Persist what the user changed before the panels go away.
    auto& settings = edp->editor->get_settings();
    for (const auto& panel : edp->editor->get_panels())
        settings.panel_visible[panel->get_title()] = panel->get_visible();
    settings.save( Editor::DEFAULT_SETTINGS_PATH );

    // Release panel GPU state and stop the import worker while the renderer is
    // still alive.
    if (auto* renderer = p_scene.get_app_ctx()->services.resolve<nc::RenderService>())
        edp->editor->shutdown( *renderer );

    world.remove_singleton<EditorPlugin>();

    unregister_gui_plugin( p_scene );
}

} // namespace sea::editor
