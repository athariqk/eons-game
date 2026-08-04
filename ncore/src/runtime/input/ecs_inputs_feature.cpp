#include "ecs_inputs_feature.h"

#include <imgui.h>

#include <ncore/core/collection.h>
#include <ncore/core/quaternion.h>
#include <ncore/modules/input/input_event.h>
#include <ncore/modules/input/input_module.h>
#include <ncore/modules/video/window_module.h>
#include <ncore/runtime/components/ecs_camera.h>
#include <ncore/runtime/components/ecs_input.h>
#include <ncore/runtime/components/ecs_transform.h>
#include <ncore/runtime/ecs_base_features.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

#if defined( NC_DEBUG )

struct InputDebugState {
    EcsQuery spatial_query;
    NSTRUCT( InputDebugState, NC_F( InputDebugState, spatial_query ) )
};

#endif

void EcsInputsFeature::build( EcsWorld& world )
{
#if defined( NC_DEBUG )
    world.emplace_singleton<InputDebugState>();

    world.system( "EcsInputsFeature::InitDebug" )
        .with<InputDebugState>()
        .in( EcsSystemPhase::INIT )
        .run( []( QueryContext& ctx ) {
            ctx.get_component<InputDebugState>()->spatial_query =
                ctx.world().query( "EcsInputsFeature::InputReceiverDebugQuery" ).with<EcsInputReceiver>().build();
        } );
#endif

    world.system( "EcsInputsFeature::Init" ).with<IoModules>().in( EcsSystemPhase::INIT ).run( []( QueryContext& ctx ) {
        auto io = ctx.get_component<IoModules>();
        io->inputs->action_bind_event( InputModule::FORWARD_ACTION_NAME, KeyEvent{ .key = Key::W } );
        io->inputs->action_bind_event( InputModule::BACKWARD_ACTION_NAME, KeyEvent{ .key = Key::S } );
        io->inputs->action_bind_event( InputModule::LEFT_ACTION_NAME, KeyEvent{ .key = Key::A } );
        io->inputs->action_bind_event( InputModule::RIGHT_ACTION_NAME, KeyEvent{ .key = Key::D } );
        io->inputs->action_bind_event( InputModule::UP_ACTION_NAME, KeyEvent{ .key = Key::SPACE } );
        io->inputs->action_bind_event( InputModule::DOWN_ACTION_NAME, KeyEvent{ .key = Key::SHIFT } );

        io->inputs->action_register( "G_LeftRoll" );
        io->inputs->action_register( "G_RightRoll" );
        io->inputs->action_bind_event( "G_LeftRoll", KeyEvent{ .key = Key::Q } );
        io->inputs->action_bind_event( "G_RightRoll", KeyEvent{ .key = Key::E } );
    } );

    world.system( "EcsInputsFeature::Update" )
        .with<IoModules>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .run( []( QueryContext& ctx ) {
            auto io = ctx.get_component<IoModules>();
            io->inputs->update();
        } );

    world.system( "EcsInputsFeature::EcsInputReceiver::KeyControl" )
        .with<EcsInputReceiver>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto io    = ctx.world().get_singleton<IoModules>();
            auto input = ctx.get_component<EcsInputReceiver>();

            input->direction.zero();

            if (ImGui::GetIO().WantCaptureKeyboard)
                return;

            Vec2 xz = io->inputs->action_get_vector(
                InputModule::LEFT_ACTION_NAME, InputModule::RIGHT_ACTION_NAME, InputModule::BACKWARD_ACTION_NAME,
                InputModule::FORWARD_ACTION_NAME
            );
            float y = io->inputs->action_get_axis( InputModule::DOWN_ACTION_NAME, InputModule::UP_ACTION_NAME );
            input->direction = Vec3( -xz.x, -y, xz.y ); // (-LeftRight, -UpDown, ForwardBackward)

            float r                = io->inputs->action_get_axis( "G_LeftRoll", "G_RightRoll" );
            input->angular_delta.z = -r * input->roll_rate;
        } );

    world.system( "EcsInputsFeature::EcsInputReceiver::MouseControl" )
        .with<EcsInputReceiver>()
        .in( EcsSystemPhase::PRE_UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto io    = ctx.world().get_singleton<IoModules>();
            auto gfx   = ctx.world().get_singleton<GraphicsModules>();
            auto input = ctx.get_component<EcsInputReceiver>();

            auto win_id = gfx->window->get_main_window_id();

            if (io->inputs->is_mouse_button_pressed( ButtonIndex::RIGHT )) {
                auto is_locked = gfx->window->window_get_mouse_locked( win_id );
                if (is_locked) {
                    gfx->window->window_set_mouse_position( win_id, gfx->window->window_get_resolution( win_id ) / 2 );
                }
                gfx->window->window_set_mouse_locked( win_id, !is_locked );
            }

            // per-frame deltas, zeroed when the mouse is unlocked
            if (gfx->window->window_get_mouse_locked( win_id )) {
                auto md                = io->inputs->get_mouse_delta();
                auto dt                = static_cast<float>( ctx.delta_time() );
                float inv_dt           = ( dt > 0.0f ) ? ( 1.0f / dt ) : 0.0f;
                input->angular_delta.x = -md.x * inv_dt * input->mouse_sensitivity;
                input->angular_delta.y = -md.y * inv_dt * input->mouse_sensitivity;
            } else {
                input->angular_delta.x = 0;
                input->angular_delta.y = 0;
            }
        } );

    world.system( "EcsInputsFeature::FlyCamUpdater" )
        .with<EcsTransform3D, EcsCamera, EcsInputReceiver>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( QueryContext& ctx, EcsEntityId ) {
            auto xform = ctx.get_component<EcsTransform3D>();
            auto input = ctx.get_component<EcsInputReceiver>();

            auto dt = static_cast<float>( ctx.delta_time() );
            xform->translation += xform->rotation * input->direction * input->magnitude * dt;

            // 6DOF camera rotation.
            // NOTE: suffers from the so called "holonomy" where if you
            // try to yaw-pitch in a circular manner, then the camera
            // gets tilted ever so slightly
            Quaternion yaw( input->angular_delta.x * dt, Vec3::up() );
            Quaternion pitch( input->angular_delta.y * dt, Vec3::right() );
            Quaternion roll( input->angular_delta.z * dt, Vec3::forward() );
            xform->rotation = xform->rotation * roll * yaw * pitch;
        } );

#if defined( NC_DEBUG )
    world.system( "EcsInputsFeature::InputDebugUI" )
        .with<IoModules>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( QueryContext& ctx ) {
            auto io    = ctx.world().get_singleton<IoModules>();
            auto state = ctx.world().get_singleton<InputDebugState>();

            if (ImGui::Begin( "Input Debug" )) {
                {
                    ImGui::SeparatorText( "Actions" );
                    DynamicArray<StringView> actions;
                    io->inputs->action_list( actions );

                    if (ImGui::BeginTable( "ActionsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg )) {
                        ImGui::TableSetupColumn( "Name" );
                        ImGui::TableSetupColumn( "Held" );
                        ImGui::TableSetupColumn( "Pressed" );
                        ImGui::TableSetupColumn( "Released" );
                        ImGui::TableHeadersRow();

                        for (const auto& action : actions) {
                            bool held     = io->inputs->action_is_held( action.data() );
                            bool pressed  = io->inputs->action_is_pressed( action.data() );
                            bool released = io->inputs->action_is_released( action.data() );

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex( 0 );
                            ImGui::Text( "%s", action.data() );
                            ImGui::TableSetColumnIndex( 1 );
                            ImGui::TextColored(
                                held ? ImVec4( 0.4f, 1.0f, 0.4f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                held ? "Yes" : "No"
                            );
                            ImGui::TableSetColumnIndex( 2 );
                            ImGui::TextColored(
                                pressed ? ImVec4( 1.0f, 1.0f, 0.3f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                pressed ? "Yes" : "No"
                            );
                            ImGui::TableSetColumnIndex( 3 );
                            ImGui::TextColored(
                                released ? ImVec4( 1.0f, 0.6f, 0.2f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                released ? "Yes" : "No"
                            );
                        }
                        ImGui::EndTable();
                    }
                }

                {
                    ImGui::SeparatorText( "Mouse Input" );
                    ImGui::DragFloat2(
                        "Pos", io->inputs->get_mouse_position().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                    ImGui::DragFloat2(
                        "Delta", io->inputs->get_mouse_delta().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                    ImGui::DragFloat2(
                        "Wheel", io->inputs->get_mouse_wheel().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                }

                {
                    ImGui::SeparatorText( "Input Receivers" );
                    for (auto& it : state->spatial_query) {
                        for (int32_t row = 0; row < it.count(); row++) {
                            it.set_row( row );
                            auto input = it.get_component<EcsInputReceiver>();
                            ImGui::Text(
                                "EID %llu | dir (%.2f, %.2f, %.2f) | mag %.2f | orient (%.2f, %.2f, %.2f)",
                                static_cast<unsigned long long>( it.entity( row ) ),
                                static_cast<double>( input->direction.x ), static_cast<double>( input->direction.y ),
                                static_cast<double>( input->direction.z ), static_cast<double>( input->magnitude ),
                                static_cast<double>( input->angular_delta.x ),
                                static_cast<double>( input->angular_delta.y ),
                                static_cast<double>( input->angular_delta.z )
                            );
                        }
                    }
                }
            }
            ImGui::End();
        } );
#endif
}

} // namespace nc
