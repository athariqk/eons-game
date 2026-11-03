// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: world_scene.cpp

#include "world_scene.h"

#if defined( DEBUG )
#include <editor/ncore_editor.h>
#endif

#include <ncore/app_context.h>
#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/runtime/resources/material.h>
#include <ncore/runtime/resources/mesh.h>
#include <ncore/runtime/resources/shader.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/config.h>

#include "water/water_mesh.h"
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
        auto io = it.world().get_singleton<IoServices>();

        if (io->inputs->is_key_pressed( Key::F5 )) {
            log::print( "Hot-reloading" );
            io->resources->load<MaterialShader>( "shaders/materials/skybox.slang", true );
            io->resources->load<MaterialShader>( "shaders/materials/water.slang", true );
            io->resources->load<MaterialShader>( "shaders/materials/water_opaque.slang", true );
        }
    } );
#endif

    get_ecs()
        .system( "FreeCamUpdater" )
        .with<ActiveCameraTag, Transform3DComponent, CameraComponent, InputComponent>()
        .in( EcsSystemPhase::UPDATE )
        .each( []( EcsIterState& it ) {
            auto xform = it.get_component<Transform3DComponent>();
            auto input = it.get_component<InputComponent>();

            auto dt = static_cast<float>( it.delta_time() );
            xform->translation += xform->rotation * input->direction * input->magnitude * dt;

            // 6DOF camera rotation.
            // NOTE: suffers from the so called "holonomy" where if you
            // try to yaw-pitch in a circular manner, then the camera
            // gets tilted ever so slightly
            // Quaternion yaw( input->angular_delta.x * dt, Vec3f::up() );
            // Quaternion pitch( input->angular_delta.y * dt, Vec3f::right() );
            // Quaternion roll( input->angular_delta.z * dt, Vec3f::forward() );
            // xform->rotation          = xform->rotation * roll * yaw * pitch;

            // i can't get the above working correctly without unwanted roll
            // so have the one below for now...

            const float yaw_amount   = input->angular_delta.x * dt;
            const float pitch_amount = input->angular_delta.y * dt;
            const float roll_amount  = input->angular_delta.z * dt;

            // FPS-style cam

            // yaw around *world* up
            Quaternion yaw( yaw_amount, Vec3f::up() );
            xform->rotation = yaw * xform->rotation;

            // pitch around *local* right
            Quaternion pitch( pitch_amount, Vec3f::right() );
            xform->rotation = xform->rotation * pitch;

            // this roll is useless as it is ignored by the world-up yaw,
            // need to find another solution
            if (!math::is_equal_approx( roll_amount, 0 )) {
                Quaternion roll( roll_amount, Vec3f::forward() );
                xform->rotation = xform->rotation * roll;
            }

            // good practice
            xform->rotation = Quaternion::normalize( xform->rotation );
        } );

    auto main_camera = root()->create_child( "MainCamera" );
    main_camera->add_component<ActiveCameraTag>();
    main_camera->add_component<Transform3DComponent>();
    main_camera->add_component<CameraComponent>( CameraComponent{ .z_near = 1.0f, .z_far = 500000.0f } );
    main_camera->add_component<InputComponent>();

    create_environment();
    create_water( main_camera );
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
    auto& res = get_resource_loader();

    skybox_cubemap = res.load<CubeMap>( "images/skybox.equirect" );
    NC_FAIL_MSG_RET( skybox_cubemap, "Failed to load skybox cubemap" );
    skybox_cubemap->prepare( *this );

    auto shader   = res.load<MaterialShader>( "shaders/materials/skybox.slang" );
    auto material = Ref<Material>::create();
    material->set_shader( shader );
    material->set_param( "cubemap", skybox_cubemap );

    constexpr Array<Vertex3D, 8> box_verts    = { Vertex3D{ -1.0f, -1.0f, -1.0f }, Vertex3D{ -1.0f, 1.0f, -1.0f },
                                                  Vertex3D{ 1.0f, 1.0f, -1.0f },   Vertex3D{ 1.0f, -1.0f, -1.0f },
                                                  Vertex3D{ -1.0f, -1.0f, 1.0f },  Vertex3D{ -1.0f, 1.0f, 1.0f },
                                                  Vertex3D{ 1.0f, 1.0f, 1.0f },    Vertex3D{ 1.0f, -1.0f, 1.0f } };
    constexpr Array<uint32_t, 36> box_indices = {
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
        0, 3, 4, 3, 7, 4
    };

    auto mesh = Ref<Mesh>::create(
        MeshDesc{
            .vertices = DynamicArray<std::byte>(
                reinterpret_cast<std::byte const*>( box_verts.data() ),
                reinterpret_cast<std::byte const*>( box_verts.data() + box_verts.size() )
            ),
            .indices       = DynamicArray<uint32_t>( box_indices.data(), box_indices.data() + box_indices.size() ),
            .vertex_stride = sizeof( Vertex3D ),
            .material      = material
        }
    );

    auto skybox = root()->create_child( "Skybox" );
    skybox->add_component<Transform3DComponent>(
        Transform3DComponent{ .translation = Vec3f(), .rotation = Quaternion::identity(), .scale = Vec3f( 80, 80, 80 ) }
    );
    skybox->add_component<MeshInstanceComponent>( MeshInstanceComponent{ .mesh = mesh } );
}

void WorldScene::create_water( Node* camera )
{
    NC_VERIFY( camera );
    NC_FAIL_MSG_RET( skybox_cubemap, "Skybox cubemap must be created before water" );

    auto cam_component = camera->get_component<CameraComponent>();

    auto water_sim = register_water_sim( this );

    auto& waves = get_wave_generator();
    NC_ASSERT( waves.get_initialized() );

    auto& res     = get_resource_loader();
    auto& app_cfg = get_app_ctx()->config;

    const uint32_t cells       = static_cast<uint32_t>( app_cfg.get_int( "Water.cells", 64 ) );
    const uint32_t levels      = static_cast<uint32_t>( app_cfg.get_int( "Water.levels", 0 ) );
    const float base_spacing   = app_cfg.get_float( "Water.base_spacing", 3.5f );
    const float horizon        = app_cfg.get_float( "Water.horizon", 450000.0f );
    const uint32_t chunk_cells = static_cast<uint32_t>( app_cfg.get_int( "Water.chunk_cells", 16 ) );

    const Color water_color            = app_cfg.get_color( "Water.water_color" );
    const Color foam_color             = app_cfg.get_color( "Water.foam_color" );
    const float roughness              = app_cfg.get_float( "Water.roughness", 0.4f );
    const float normal_strength        = app_cfg.get_float( "Water.normal_strength", 1.0f );
    const float env_strength           = app_cfg.get_float( "Water.env_strength", 0.5f );
    const Vec3f absorption             = app_cfg.get_vec3( "Water.absorption" );
    const float horizon_fade_threshold = app_cfg.get_float( "Water.horizon_fade_scale", 0.3f );
    const float morph_width            = app_cfg.get_float( "Water.morph_transition_width", 0.1f );

    auto shader   = res.load<MaterialShader>( "shaders/materials/water.slang" );
    auto material = Ref<Material>::create();
    material->set_shader( shader );
    material->set_param( "displacementMap", waves.get_displacement_map() );
    material->set_param( "normalMap", waves.get_normal_map() );
    material->set_param( "envMap", skybox_cubemap );
    material->set_param( "waterColor", water_color.normalize() );
    material->set_param( "foamColor", foam_color.normalize() );
    material->set_param( "roughness", roughness );
    material->set_param( "normalStrength", normal_strength );
    material->set_param( "envStrength", env_strength );
    material->set_param( "mapSize", static_cast<float>( waves.get_map_size() ) );
    material->set_param( "absorption", absorption );
    material->set_param( "horizonDistance", cam_component->z_far * horizon_fade_threshold );
    material->set_param( "morphTransitionWidth", morph_width );

    auto* sim            = water_sim->get_component<WaterSimComponent>();
    sim->material        = material;
    water_sim->notify_changed<WaterSimComponent>();

    auto water_mesh = create_water_clipmap(
        WaterMeshDesc{
            .cells        = cells,
            .levels       = levels,
            .base_spacing = base_spacing,
            .horizon      = horizon,
            .chunk_cells  = chunk_cells
        }
    );

    auto plane = water_sim->create_child( "WaterPlane" );
    plane->add_component<Transform3DComponent>( Transform3DComponent{
        .translation      = Vec3f(),
        .rotation         = Quaternion::identity(),
        .scale            = Vec3f( 1, 1, 1 ),
        .inherit_position = false, // Global must be exactly the local transform
        .inherit_rotation = false,
        .inherit_scale    = false,
    } );
    plane->add_component<Constraint3DComponent>( Constraint3DComponent{
        .lock_position_x = false,
        .lock_position_y = true,
        .lock_position_z = false,
    } );
    plane->add_component<MeshInstanceComponent>( MeshInstanceComponent{ .mesh = water_mesh } );
    plane->add_component<MaterialOverrideComponent>(
        MaterialOverrideComponent{ .material = material, .override_mesh_material = true }
    );
}

} // namespace sea
