#include "world_scene.h"

#if defined( DEBUG )
#include <editor/ncore_editor.h>
#endif
#include <ncore/application.h>
#include <ncore/resources/resource.h>
#include <ncore/runtime/components/camera.h>
#include <ncore/runtime/components/input.h>
#include <ncore/runtime/components/material.h>
#include <ncore/runtime/components/mesh.h>
#include <ncore/runtime/components/resource.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>

#include "tests/test_compute_shader.h"
#include "water/water_sim.h"
#include "water/wave_cascade_params.h"
#include "water/wave_generator.h"

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
            io->Resources->load<MaterialShader>( "shaders/materials/skybox.slang", true );
            io->Resources->load<MaterialShader>( "shaders/materials/water.slang", true );
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

            const float yaw_amount   = input->AngularDelta.x * dt;
            const float pitch_amount = input->AngularDelta.y * dt;
            const float roll_amount  = input->AngularDelta.z * dt;

            Quaternion yaw( yaw_amount, Vec3::up() );
            xform->Rotation = yaw * xform->Rotation;

            Quaternion pitch( pitch_amount, Vec3::right() );
            xform->Rotation = xform->Rotation * pitch;

            if (!math::is_equal_approx( roll_amount, 0 )) {
                Quaternion roll( roll_amount, Vec3::forward() );
                xform->Rotation = xform->Rotation * roll;
            }

            xform->Rotation = Quaternion::normalize( xform->Rotation );
        } );

    auto main_camera = root()->create_child( "MainCamera" );
    main_camera->add_component<ActiveCameraTag>();
    main_camera->add_component<Transform3DComponent>();
    main_camera->add_component<CameraComponent>();
    main_camera->add_component<InputComponent>();
}

void WorldScene::on_exit()
{
#if defined( DEBUG )
    editor::unregister_editor_plugin( *this );
#endif
    Scene::on_exit();
}

//------------------------------------------------------------------------------

void WorldScene::create_environment()
{
    auto& res = get_resource_loader();
    auto rd   = get_app_ctx()->Services.resolve<RenderService>();

    constexpr Array<Vertex3D, 8> box_verts    = { Vertex3D{ -1.0f, -1.0f, -1.0f }, Vertex3D{ -1.0f, 1.0f, -1.0f },
                                                  Vertex3D{ 1.0f, 1.0f, -1.0f },   Vertex3D{ 1.0f, -1.0f, -1.0f },
                                                  Vertex3D{ -1.0f, 1.0f, 1.0f },  Vertex3D{ -1.0f, 1.0f, 1.0f },
                                                  Vertex3D{ 1.0f, 1.0f, 1.0f },    Vertex3D{ 1.0f, -1.0f, 1.0f } };
    constexpr Array<uint16_t, 36> box_indices = {
        0, 1, 2, 0, 2, 3, 4, 7, 6, 4, 6, 5, 0, 4, 1, 1, 4, 5,
        2, 6, 3, 3, 6, 7, 1, 5, 2, 2, 5, 6, 0, 4, 3, 3, 7, 4
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
    auto skybox_mesh_rid = res.add( skybox_mesh );

    auto equirect   = res.load<Image>( "images/skybox.png" );
    auto cube_map   = Ref<CubeMap>::create( equirect, equirect->get_width() / 4 );
    auto skybox_tex = rd->texture_cube_create( *cube_map );

    skybox_cubemap_rid_ = skybox_tex;

    MaterialComponent skybox_mat;
    skybox_mat.Shader = res.load( "shaders/materials/skybox.slang" );
    skybox_mat.add_texture( skybox_tex );

    auto skybox = root()->create_child( "Skybox" );
    skybox->add_component<Transform3DComponent>(
        Transform3DComponent{ .Translation = Vec3(), .Rotation = Quaternion::identity(), .Scale = Vec3( 80, 80, 80 ) }
    );
    skybox->add_component<HasResourceTag>();
    skybox->add_component<MeshComponent>( MeshComponent{ .Source = skybox_mesh_rid } );
    skybox->add_component<MeshRenderComponent>();
    skybox->add_component<MaterialComponent>( skybox_mat );
    skybox->add_component<MaterialRenderComponent>();
}

void WorldScene::create_water()
{
    register_water_sim( *this );

    auto& waves = get_wave_generator();
    NC_ASSERT( waves.is_initialized() );

    auto& res = get_resource_loader();

    // Keep in sync with water_sim.cpp cascade tile lengths / scales.
    const WaveCascadeParams cascade0 = [] {
        WaveCascadeParams c{};
        c.tile_length = { 50.0f, 50.0f };
        return c;
    }();
    const WaveCascadeParams cascade1 = [] {
        WaveCascadeParams c{};
        c.tile_length        = { 17.0f, 17.0f };
        c.displacement_scale = 0.6f;
        c.normal_scale       = 0.6f;
        return c;
    }();

    MaterialComponent water_mat;
    water_mat.Shader = res.load( "shaders/materials/water.slang" );
    water_mat.add_texture( waves.displacement_map() );
    water_mat.add_texture( waves.normal_map() );
    water_mat.add_texture( skybox_cubemap_rid_ );

    Vec4 map_scales[2];
    map_scales[0] = {
        1.0f / cascade0.tile_length.x,
        1.0f / cascade0.tile_length.y,
        cascade0.displacement_scale,
        cascade0.normal_scale,
    };
    map_scales[1] = {
        1.0f / cascade1.tile_length.x,
        1.0f / cascade1.tile_length.y,
        cascade1.displacement_scale,
        cascade1.normal_scale,
    };
    water_mat.set_params<Vec4[2]>( "mapScales", map_scales );

    // Defaults aligned with GodotOceanWaves global water_color / foam_color / roughness.
    const Vec3 water_color = { 0.02f, 0.12f, 0.18f };
    const Vec3 foam_color  = { 0.92f, 0.95f, 0.98f };
    const float roughness        = 0.4f;
    const float normal_strength  = 1.0f;
    const float reflection_strength = 1.0f;
    water_mat.set_params( "waterColor", water_color );
    water_mat.set_params( "foamColor", foam_color );
    water_mat.set_params( "roughness", roughness );
    water_mat.set_params( "normalStrength", normal_strength );
    water_mat.set_params( "reflectionStrength", reflection_strength );

    auto mesh     = Ref<PlaneMesh>::create( 128, 128 );
    auto mesh_rid = res.add( mesh );
    auto plane    = root()->create_child( "WaterPlane" );
    plane->add_component<Transform3DComponent>(
        Transform3DComponent{ .Translation = Vec3(), .Rotation = Quaternion::identity(), .Scale = Vec3( 50, 1, 50 ) }
    );
    auto plane_mesh = plane->create_child( "WaterMesh" );
    plane_mesh->add_component<HasResourceTag>();
    plane_mesh->add_component<MeshComponent>( MeshComponent{ .Source = mesh_rid } );
    plane_mesh->add_component<MeshRenderComponent>();
    plane_mesh->add_component<MaterialComponent>( water_mat );
    plane_mesh->add_component<MaterialRenderComponent>();
}

} // namespace sea
