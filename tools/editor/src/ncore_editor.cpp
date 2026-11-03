#include <editor/ncore_editor.h>
#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/log.h>

#include "editor.h"
#include "editor_components.h"
#include "plugins/gui_plugin.h"

namespace nc::editor {

void NCAPI_EDITOR register_editor_plugin( Scene& scene )
{
    auto& world = scene.get_ecs();
    NC_FAIL_MSG_RET( !world.get_singleton<EditorPlugin>(), "Editor plugin is already registered." );

    register_editor_component_types( scene );

    register_gui_plugin( scene );

    world.add_singleton<EditorPlugin>();
    world.get_singleton<EditorPlugin>()->editor = std::make_unique<Editor>( scene );

    // The editor camera only consumes input while a 3D viewport is hovered or
    // focused; without that gate the camera flies while the user drags panels.
    scene.get_ecs()
        .system( "EditorCamera_FreeCamUpdater" )
        .with<EditorCameraTag, Transform3DComponent, CameraComponent, InputComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto io    = it.world().get_singleton_const<IoServices>();
            auto edp   = it.world().get_singleton_const<EditorPlugin>();
            auto xform = it.get_component<Transform3DComponent>();
            auto input = it.get_component<InputComponent>();

            const bool ui_owns_keyboard = io && io->inputs && io->inputs->is_ui_keyboard_captured();
            const bool viewport_active =
                edp->editor->get_frame().any_viewport_hovered || edp->editor->get_frame().any_viewport_focused;
            if (!viewport_active || ui_owns_keyboard) {
                input->direction.zero();
                input->angular_delta.zero();
                return;
            }

            auto dt = static_cast<float>( it.delta_time() );
            xform->translation += xform->rotation * input->direction * input->magnitude * dt;

            const float yaw_amount   = input->angular_delta.x * dt;
            const float pitch_amount = input->angular_delta.y * dt;
            const float roll_amount  = input->angular_delta.z * dt;

            // FPS-style cam: yaw around *world* up, pitch around *local* right.
            Quaternion yaw( yaw_amount, Vec3f::up() );
            xform->rotation = yaw * xform->rotation;

            Quaternion pitch( pitch_amount, Vec3f::right() );
            xform->rotation = xform->rotation * pitch;

            // This roll is ignored by the world-up yaw; kept until 6DOF lands.
            if (!math::is_equal_approx( roll_amount, 0.f )) {
                Quaternion roll( roll_amount, Vec3f::forward() );
                xform->rotation = xform->rotation * roll;
            }

            // good practice
            xform->rotation = Quaternion::normalize( xform->rotation );
        } );

    auto editor_cam = scene.root()->create_child( "EditorCamera" );
    editor_cam->add_component<EditorCameraTag>();
    // Spawn outside the demo scene contents: at the origin the camera sits inside
    // the unit cube at (0,0,0), so back-face culling renders a black viewport.
    editor_cam->add_component<Transform3DComponent>( Transform3DComponent{
        .translation = Vec3f( 0, 2, 6 ), .rotation = Quaternion::identity(), .scale = Vec3f( 1, 1, 1 )
    } );
    editor_cam->add_component<CameraComponent>( CameraComponent{ .z_far = 5000.0f } );
    editor_cam->add_component<InputComponent>();

    // ImGuizmo needs the ImGui context that the GUI plugin created.
    scene.get_ecs()
        .system( "EditorImGuiInit" )
        .with<GuiStateComponent>()
        .in( EcsSystemPhase::INIT )
        .order( 20 )
        .run( []( EcsIterState& it ) {
            ImGuizmo::SetImGuiContext( it.get_component<GuiStateComponent>()->imgui_ctx );
        } );

    scene.get_ecs()
        .system( "EditorImGuizmoBeginFrame" )
        .with<GuiStateComponent>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .order( -900 )
        .run( []( EcsIterState& ) {
            ImGuizmo::SetOrthographic( false );
            ImGuizmo::BeginFrame();
        } );

    // Must run before every other PRE_UPDATE system that submits a window: the
    // dockspace node has to exist before the first Begin() of the frame.
    scene.get_ecs()
        .system( "EditorDockspace" )
        .with<GuiStateComponent, EditorPlugin>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .order( -950 )
        .run( []( EcsIterState& it ) { it.get_component<EditorPlugin>()->editor->draw_dockspace(); } );

    // After NewFrame (-1000) and ImGuizmo::BeginFrame (-900), before the
    // engine's camera update (0) so render targets are bound in time.
    scene.get_ecs()
        .system( "EditorUi" )
        .with<EditorPlugin>()
        .in( EcsSystemPhase::UPDATE )
        .order( -30 )
        .run( []( EcsIterState& it ) {
            auto edp = it.get_component<EditorPlugin>();
            edp->editor->begin_frame();
            edp->editor->draw();
        } );

    // ImGui resolves hover/capture state by the end of the frame.
    scene.get_ecs()
        .system( "EditorUiCapture" )
        .with<EditorPlugin>()
        .in( EcsSystemPhase::POST_UPDATE )
        .run( []( EcsIterState& it ) { it.get_component<EditorPlugin>()->editor->publish_ui_capture(); } );

    scene.get_ecs()
        .system( "EditorCanvasPass" )
        .with<VideoServices>()
        .in( EcsSystemPhase::POST_FRAME )
        .order( 15 )
        .run( [editor_cam]( EcsIterState& it ) {
            auto vid       = it.get_component<VideoServices>();
            RID cam_source = editor_cam->get_component<CameraComponent>()->source;

            RenderService::RenderTargetDesc desc;
            desc.camera       = cam_source;
            desc.draw_spatial = false;
            desc.draw_canvas  = true;
            desc.to_screen    = true;
            vid->renderer->render_target( desc );
        } );
}

void NCAPI_EDITOR unregister_editor_plugin( Scene& scene )
{
    auto& world = scene.get_ecs();
    auto edp    = world.get_singleton_const<EditorPlugin>();
    NC_FAIL_MSG_RET( edp, "Editor plugin is already unregistered" );

    // Persist what the user changed before the panels go away.
    auto& settings = edp->editor->get_settings();
    for (const auto& panel : edp->editor->get_panels())
        settings.panel_visible[panel->get_title()] = panel->get_visible();
    settings.save( "edp.ini" );

    edp->editor->release_gpu();

    world.remove_singleton<EditorPlugin>();

    unregister_gui_plugin( scene );
}

} // namespace nc::editor
