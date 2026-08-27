#include "editor_camera.h"

#include <ncore/runtime/components/camera.h>
#include <ncore/runtime/components/input.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/render_service.h>

#include "editor_state.h"

namespace nc::editor {

struct EditorCameraTag {
    NSTRUCT1( EditorCameraTag )
};

void register_editor_camera( Scene& scene )
{
    scene.get_ecs()
        .system( "EditorCamera_CaptureCamSource" )
        .with<EditorCameraTag, CameraComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto state             = it.world().get_singleton<EditorState>();
            auto cam               = it.get_component<CameraComponent>();
            state->EditorCamSource = cam->Source;
        } );

    scene.get_ecs()
        .system( "EditorCamera_UpdateEditorCam" )
        .with<EditorCameraTag, CameraComponent, Transform3DComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto cam   = it.get_component<CameraComponent>();
            auto xform = it.get_component<Transform3DComponent>();
            auto state = it.world().get_singleton<EditorState>();

            if (state->ViewportRT) {
                cam->RenderTexture = state->ViewportRT;
                cam->DepthTexture  = state->ViewportDT;
                cam->DisplayRect   = Rect2i( 0, 0, state->ViewportSize.x, state->ViewportSize.y );
            }
            cam->RenderToScreen = false;
            cam->DrawCanvas     = false;
        } );

    scene.get_ecs()
        .system( "EditorCamera_FreeCamUpdater" )
        .with<EditorCameraTag, Transform3DComponent, CameraComponent, InputComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto xform = it.get_component<Transform3DComponent>();
            auto input = it.get_component<InputComponent>();
            auto cam   = it.get_component<CameraComponent>();

            auto dt = static_cast<float>( it.delta_time() );
            xform->Translation += xform->Rotation * input->Direction * input->Magnitude * dt;

            // 6DOF camera rotation.
            // NOTE: suffers from the so called "holonomy" where if you
            // try to yaw-pitch in a circular manner, then the camera
            // gets tilted ever so slightly
            // Quaternion yaw( input->angular_delta.x * dt, Vec3::up() );
            // Quaternion pitch( input->angular_delta.y * dt, Vec3::right() );
            // Quaternion roll( input->angular_delta.z * dt, Vec3::forward() );
            // xform->Rotation          = xform->Rotation * roll * yaw * pitch;

            // i can't get the above working correctly without unwanted roll
            // so have the one below for now...

            const float yaw_amount   = input->AngularDelta.x * dt;
            const float pitch_amount = input->AngularDelta.y * dt;
            const float roll_amount  = input->AngularDelta.z * dt;

            // FPS-style cam

            // yaw around *world* up
            Quaternion yaw( yaw_amount, Vec3::up() );
            xform->Rotation = yaw * xform->Rotation;

            // pitch around *local* right
            Quaternion pitch( pitch_amount, Vec3::right() );
            xform->Rotation = xform->Rotation * pitch;

            // this roll is useless as it is ignored by the world-up yaw,
            // need to find another solution
            if (!math::is_equal_approx( roll_amount, 0 )) {
                Quaternion roll( roll_amount, Vec3::forward() );
                xform->Rotation = xform->Rotation * roll;
            }

            // good practice
            xform->Rotation = Quaternion::normalize( xform->Rotation );
        } );

    scene.get_ecs()
        .system( "EditorCamera_RedirectGameCameras" )
        .with<ActiveCameraTag, CameraComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto cam   = it.get_component<CameraComponent>();
            auto state = it.world().get_singleton<EditorState>();
            auto vid   = it.world().get_singleton<VideoServices>();

            if (state->ViewportSize.x > 0 && state->ViewportSize.y > 0) {
                if (!state->GameViewRT || state->ViewportSize != state->GameViewSize) {
                    if (state->GameViewRT)
                        vid->Gfx->destroy_rid( state->GameViewRT );
                    if (state->GameViewDT)
                        vid->Gfx->destroy_rid( state->GameViewDT );

                    Vec2i gv_size(
                        static_cast<int>( state->ViewportSize.x ), static_cast<int>( state->ViewportSize.y )
                    );
                    state->GameViewRT   = vid->Gfx->texture_render_create( gv_size, TextureFormat::RGBA8_UNORM_SRGB );
                    state->GameViewDT   = vid->Gfx->texture_render_create( gv_size, TextureFormat::D32_FLOAT );
                    state->GameViewSize = state->ViewportSize;
                }

                cam->RenderTexture = state->GameViewRT;
                cam->DepthTexture  = state->GameViewDT;
                cam->DisplayRect   = Rect2i(
                    0, 0, static_cast<int>( state->GameViewSize.x ), static_cast<int>( state->GameViewSize.y )
                );
                cam->RenderToScreen = false;
                cam->DrawCanvas     = false;
            }
        } );

    auto editor_cam = scene.root()->create_child( "EditorCamera" );
    editor_cam->add_component<EditorCameraTag>();
    editor_cam->add_component<Transform3DComponent>(
        Transform3DComponent{ Vec3( 0, 0, 0 ), Quaternion::identity(), Vec3( 1, 1, 1 ) }
    );
    editor_cam->add_component<CameraComponent>();
    editor_cam->add_component<InputComponent>();
}

} // namespace nc::editor
