#include "world_scene.h"

#if defined( DEBUG )
#include <editor/ncore_editor.h>
#endif
#include <ncore/application.h>
#include <ncore/resources/cube_map.h>
#include <ncore/resources/image.h>
#include <ncore/resources/material_template.h>
#include <ncore/resources/mesh.h>
#include <ncore/runtime/components/camera.h>
#include <ncore/runtime/components/input.h>
#include <ncore/runtime/components/material.h>
#include <ncore/runtime/components/mesh.h>
#include <ncore/runtime/components/resource.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>

namespace sea {

using namespace nc;

void WorldScene::on_ready()
{
#if defined( DEBUG )
    editor::register_editor_plugin( *this );

    get_ecs().system( "HotReload" ).in( EcsSystemPhase::UPDATE ).run( []( EcsIterState& it ) {
        auto io = it.world().get_singleton<IOServices>();

        if (io->Inputs->is_key_pressed( Key::F5 )) {
            log::print( "Hot-reloading" );
            io->Resources->load<MaterialTemplate>( "shaders/skybox.slang", true );
            io->Resources->load<MaterialTemplate>( "shaders/water.slang", true );
            io->Resources->load<MaterialTemplate>( "materials/skybox.material", true );
            io->Resources->load<MaterialTemplate>( "materials/water.material", true );
        }
    } );
#endif

    create_environment();
    create_water();

    get_ecs()
        .system( "FreeCamUpdater" )
        .with<ActiveCameraTag, Transform3DComponent, CameraComponent, InputComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto xform = it.get_component<Transform3DComponent>();
            auto input = it.get_component<InputComponent>();

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

    auto main_camera = root()->create_child( "MainCamera" );
    main_camera->add_component<ActiveCameraTag>();
    main_camera->add_component<Transform3DComponent>(
        Transform3DComponent{ Vec3( 0, 0, 5 ), Quaternion::identity(), Vec3( 1, 1, 1 ) }
    );
    main_camera->add_component<CameraComponent>();
    main_camera->add_component<InputComponent>();
}

void WorldScene::on_exit()
{
#if defined( DEBUG )
    editor::unregister_editor_plugin( *this ); // must come before subsequent ImGui context destruction
#endif
    Scene::on_exit();
}

//------------------------------------------------------------------------------

void WorldScene::create_environment()
{
    auto res = get_app_ctx()->Services.resolve<ResourceService>();
    auto rd  = get_app_ctx()->Services.resolve<RenderService>();

    constexpr Array<Vertex3D, 8> box_verts    = { Vertex3D{ -1.0f, -1.0f, -1.0f }, Vertex3D{ -1.0f, 1.0f, -1.0f },
                                                  Vertex3D{ 1.0f, 1.0f, -1.0f },   Vertex3D{ 1.0f, -1.0f, -1.0f },
                                                  Vertex3D{ -1.0f, -1.0f, 1.0f },  Vertex3D{ -1.0f, 1.0f, 1.0f },
                                                  Vertex3D{ 1.0f, 1.0f, 1.0f },    Vertex3D{ 1.0f, -1.0f, 1.0f } };
    constexpr Array<uint16_t, 36> box_indices = {
        // Front (z = -1)
        0, 1, 2, 0, 2, 3,
        // Back (z = +1)
        4, 7, 6, 4, 6, 5,
        // Left (x = -1)
        0, 4, 1, 1, 4, 5,
        // Right (x = +1)
        2, 6, 3, 3, 6, 7,
        // Top (y = +1)
        1, 5, 2, 2, 5, 6,
        // Bottom (y = -1)
        0, 4, 3, 3, 7, 4
    };

    auto skybox_mesh = Ref<Mesh>::create(
        MeshDesc{
            .vertices = DynamicArray<std::byte>(
                reinterpret_cast<std::byte const*>( box_verts.data() ),
                reinterpret_cast<std::byte const*>( box_verts.data() + box_verts.size() )
            ),
            .indices       = DynamicArray<uint16_t>( box_indices.data(), box_indices.data() + box_indices.size() ),
            .vertex_stride = sizeof( Vertex3D )
        }
    );
    auto skybox_mesh_rid = res->add( skybox_mesh );

    auto equirect   = res->load<Image>( "images/skybox.png" );
    auto cube_map   = Ref<CubeMap>::create( equirect, equirect->get_width() / 4 );
    auto skybox_tex = rd->texture_cube_create( *cube_map );

    MaterialComponent skybox_mat;
    skybox_mat.Source = res->load( "materials/skybox.material" );
    skybox_mat.add_texture( skybox_tex );

    auto skybox = root()->create_child( "Skybox" );
    skybox->add_component<HasResourceTag>();
    skybox->add_component<MeshComponent>( MeshComponent{ skybox_mesh_rid } );
    skybox->add_component<MaterialComponent>( skybox_mat );
}

void WorldScene::create_water()
{
    auto res_svc = get_app_ctx()->Services.resolve<ResourceService>();

    auto mesh     = Ref<PlaneMesh>::create( 16, 16 );
    auto mesh_rid = res_svc->add( mesh );
    auto plane    = root()->create_child( "WaterPlane" );
    plane->add_component<Transform3DComponent>(
        Transform3DComponent{ Vec3( 0, -3, 0 ), Quaternion::identity(), Vec3( 5, 1, 5 ) }
    );
    auto plane_mesh = plane->create_child( "WaterMesh" );
    plane_mesh->add_component<HasResourceTag>();
    plane_mesh->add_component<MeshComponent>( MeshComponent{ mesh_rid } );
    plane_mesh->add_component<MaterialComponent>( MaterialComponent{ res_svc->load( "materials/water.material" ) } );
}

} // namespace sea
