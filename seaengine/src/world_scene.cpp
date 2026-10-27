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
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/services/io/input_service.h>
#include <ncore/services/service_registry.h>
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
        auto io = it.world().get_singleton<IOServices>();

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
    auto rd   = get_app_ctx()->services.resolve<RenderService>();

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

    auto skybox_mesh = Ref<Mesh>::create(
        MeshDesc{
            .vertices = DynamicArray<std::byte>(
                reinterpret_cast<std::byte const*>( box_verts.data() ),
                reinterpret_cast<std::byte const*>( box_verts.data() + box_verts.size() )
            ),
            .indices       = DynamicArray<uint32_t>( box_indices.data(), box_indices.data() + box_indices.size() ),
            .vertex_stride = sizeof( Vertex3D )
        }
    );
    auto skybox_mesh_rid = res.add( skybox_mesh );

    auto cubemap = res.load<CubeMap>( "images/skybox.equirect" );
    skybox_tex   = rd->texture_cube_create( *cubemap );

    MaterialComponent skybox_mat;
    skybox_mat.shader = res.load( "shaders/materials/skybox.slang" );
    skybox_mat.add_texture( skybox_tex );

    auto skybox = root()->create_child( "Skybox" );
    skybox->add_component<Transform3DComponent>(
        Transform3DComponent{ .translation = Vec3f(), .rotation = Quaternion::identity(), .scale = Vec3f( 80, 80, 80 ) }
    );
    skybox->add_component<HasResourceTag>();
    skybox->add_component<MeshComponent>( MeshComponent{ .source = skybox_mesh_rid } );
    skybox->add_component<MeshRenderComponent>();
    skybox->add_component<MaterialComponent>( skybox_mat );
    skybox->add_component<MaterialRenderComponent>();
}

static void apply_cascade_params( const WaterSimComponent& sim, MaterialComponent& mat )
{
    MapScale scales[8] = {};
    for (int i = 0; i < sim.cascade_count; ++i) {
        const auto& c                = sim.cascades[i];
        scales[i].tile_length        = Vec2f( 1.0f / c.scale.tile_length.x, 1.0f / c.scale.tile_length.y );
        scales[i].displacement_scale = c.scale.displacement_scale;
        scales[i].normal_scale       = c.scale.normal_scale;
    }
    mat.set_params( "numCascades", sim.cascade_count );
    mat.set_params( "mapScales", scales );
};

void WorldScene::create_water( Node* camera )
{
    NC_VERIFY( camera );

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
    plane->add_component<HasResourceTag>();
    plane->add_component<MeshComponent>( MeshComponent{ .source = res.add( water_mesh ) } );
    plane->add_component<MeshRenderComponent>();

    MaterialComponent water_mat;
    water_mat.shader = res.load( "shaders/materials/water.slang" );
    water_mat.add_texture( waves.get_displacement_map() );
    water_mat.add_texture( waves.get_normal_map() );
    water_mat.add_texture( skybox_tex );
    water_mat.set_params( "waterColor", water_color.normalize() );
    water_mat.set_params( "foamColor", foam_color.normalize() );
    water_mat.set_params( "roughness", roughness );
    water_mat.set_params( "normalStrength", normal_strength );
    water_mat.set_params( "envStrength", env_strength );
    water_mat.set_params( "mapSize", static_cast<float>( waves.get_map_size() ) );
    water_mat.set_params( "absorption", absorption );
    water_mat.set_params( "horizonDistance", cam_component->z_far * horizon_fade_threshold );
    water_mat.set_params( "morphTransitionWidth", morph_width );
    apply_cascade_params( *water_sim->get_component<WaterSimComponent>(), water_mat );
    plane->add_component<MaterialComponent>( water_mat );
    plane->add_component<MaterialRenderComponent>();

    get_ecs()
        .observer( "WaterSim_Material_Setter" )
        .event( ecs::ON_SET )
        .with<WaterSimComponent>()
        .up()
        .with<MaterialComponent>()
        .filter()
        .each( []( EcsIterState& it ) {
            auto sim = it.get_component<WaterSimComponent>();
            auto mat = it.get_entity_component<MaterialComponent>();
            apply_cascade_params( *sim, *mat );
            it.notify_changed<MaterialComponent>();
        } );
}

} // namespace sea
