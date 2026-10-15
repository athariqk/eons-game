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

#include "tests/test_compute_shader.h"
#include "water/polar_mesh.h"
#include "water/water_sim.h"
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
            io->Resources->load<MaterialShader>( "shaders/materials/water_opaque.slang", true );
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
            xform->Translation += xform->Rotation * input->Direction * input->Magnitude * dt;

            // 6DOF camera rotation.
            // NOTE: suffers from the so called "holonomy" where if you
            // try to yaw-pitch in a circular manner, then the camera
            // gets tilted ever so slightly
            // Quaternion yaw( input->angular_delta.x * dt, Vec3f::up() );
            // Quaternion pitch( input->angular_delta.y * dt, Vec3f::right() );
            // Quaternion roll( input->angular_delta.z * dt, Vec3f::forward() );
            // xform->Rotation          = xform->Rotation * roll * yaw * pitch;

            // i can't get the above working correctly without unwanted roll
            // so have the one below for now...

            const float yaw_amount   = input->AngularDelta.x * dt;
            const float pitch_amount = input->AngularDelta.y * dt;
            const float roll_amount  = input->AngularDelta.z * dt;

            // FPS-style cam

            // yaw around *world* up
            Quaternion yaw( yaw_amount, Vec3f::up() );
            xform->Rotation = yaw * xform->Rotation;

            // pitch around *local* right
            Quaternion pitch( pitch_amount, Vec3f::right() );
            xform->Rotation = xform->Rotation * pitch;

            // this roll is useless as it is ignored by the world-up yaw,
            // need to find another solution
            if (!math::is_equal_approx( roll_amount, 0 )) {
                Quaternion roll( roll_amount, Vec3f::forward() );
                xform->Rotation = xform->Rotation * roll;
            }

            // good practice
            xform->Rotation = Quaternion::normalize( xform->Rotation );
        } );

    auto main_camera = root()->create_child( "MainCamera" );
    main_camera->add_component<ActiveCameraTag>();
    main_camera->add_component<Transform3DComponent>();
    main_camera->add_component<CameraComponent>( CameraComponent{ .zNear = 1.0f, .zFar = 500000.0f } );
    main_camera->add_component<InputComponent>();

    create_environment();
    create_water( main_camera );
    create_submerged_objects();
    // hello_world_compute_shader( *this );
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
    auto rd   = get_app_ctx()->Services.resolve<RenderService>();

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
        0, 3, 4, 3, 7, 4
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

    auto cubemap = res.load<CubeMap>( "images/skybox.equirect" );
    skybox_tex   = rd->texture_cube_create( *cubemap );

    MaterialComponent skybox_mat;
    skybox_mat.Shader = res.load( "shaders/materials/skybox.slang" );
    skybox_mat.add_texture( skybox_tex );

    auto skybox = root()->create_child( "Skybox" );
    skybox->add_component<Transform3DComponent>(
        Transform3DComponent{ .Translation = Vec3f(), .Rotation = Quaternion::identity(), .Scale = Vec3f( 80, 80, 80 ) }
    );
    skybox->add_component<HasResourceTag>();
    skybox->add_component<MeshComponent>( MeshComponent{ .Source = skybox_mesh_rid } );
    skybox->add_component<MeshRenderComponent>();
    skybox->add_component<MaterialComponent>( skybox_mat );
    skybox->add_component<MaterialRenderComponent>();
}

static void apply_cascade_params( const WaterSimComponent& sim, MaterialComponent& mat )
{
    MapScale scales[8] = {};
    for (int i = 0; i < sim.CascadeCount; ++i) {
        const auto& c                = sim.Cascades[i];
        scales[i].tile_length        = Vec2f( 1.0f / c.scale.tile_length.x, 1.0f / c.scale.tile_length.y );
        scales[i].displacement_scale = c.scale.displacement_scale;
        scales[i].normal_scale       = c.scale.normal_scale;
    }
    mat.set_params( "numCascades", sim.CascadeCount );
    mat.set_params( "mapScales", scales );
};

void WorldScene::create_water( Node* camera )
{
    NC_VERIFY( camera );

    auto cam_component = camera->get_component<CameraComponent>();

    auto water_sim = register_water_sim( camera );

    auto& waves = get_wave_generator();
    NC_ASSERT( waves.is_initialized() );

    auto& res     = get_resource_loader();
    auto& app_cfg = get_app_ctx()->Config;

    const Color water_color            = app_cfg.get_color( "Water.WaterColor" );
    const Color foam_color             = app_cfg.get_color( "Water.FoamColor" );
    const float roughness              = app_cfg.get_float( "Water.Roughness", 0.4f );
    const float normal_strength        = app_cfg.get_float( "Water.NormalStrength", 1.0f );
    const float env_strength           = app_cfg.get_float( "Water.EnvStrength", 0.5f );
    const Vec3f absorption             = app_cfg.get_vec3( "Water.Absorption" );
    const float horizon_radius         = app_cfg.get_float( "Water.HorizonRadius", 450000.0f );
    const float horizon_rings          = app_cfg.get_float( "Water.HorizonRings", 140.0f );
    const float horizon_segments       = app_cfg.get_float( "Water.HorizonSegments", 160.0f );
    const float ring_growth            = app_cfg.get_float( "Water.RingGrowth", 1.18f );
    const float inner_radius           = app_cfg.get_float( "Water.InnerRadius", 3.0f );
    const float near_radius            = app_cfg.get_float( "Water.NearRadius", 250.0f );
    const float near_spacing           = app_cfg.get_float( "Water.NearSpacing", 4.0f );
    const float horizon_fade_threshold = app_cfg.get_float( "Water.HorizonFadeScale", 0.3f );

    auto plane = water_sim->create_child( "WaterPlane" );
    plane->add_component<Transform3DComponent>( Transform3DComponent{
        .Translation     = Vec3f(),
        .Rotation        = Quaternion::identity(),
        .Scale           = Vec3f( 1, 1, 1 ),
        .InheritPosition = true,
        .InheritRotation = false,
        .InheritScale    = false,
    } );
    plane->add_component<Transform3DConstraintsComponent>( Transform3DConstraintsComponent{
        .LockPositionX = false,
        .LockPositionY = true,
        .LockPositionZ = false,
    } );
    plane->add_component<HasResourceTag>();

    auto mesh = create_polar_plane_mesh(
        static_cast<uint32_t>( horizon_segments ), static_cast<uint32_t>( horizon_rings ), inner_radius, horizon_radius,
        ring_growth, near_radius, near_spacing
    );
    plane->add_component<MeshComponent>( MeshComponent{ .Source = res.add( mesh ) } );

    plane->add_component<MeshRenderComponent>();

    MaterialComponent water_mat;
    water_mat.Shader = res.load( "shaders/materials/water.slang" );
    water_mat.add_texture( waves.displacement_map() );
    water_mat.add_texture( waves.normal_map() );
    water_mat.add_texture( skybox_tex );
    water_mat.set_params( "waterColor", water_color.normalize() );
    water_mat.set_params( "foamColor", foam_color.normalize() );
    water_mat.set_params( "roughness", roughness );
    water_mat.set_params( "normalStrength", normal_strength );
    water_mat.set_params( "envStrength", env_strength );
    water_mat.set_params( "mapSize", static_cast<float>( waves.map_size() ) );
    water_mat.set_params( "absorption", absorption );
    water_mat.set_params( "horizonDistance", cam_component->zFar * horizon_fade_threshold );
    apply_cascade_params( *water_sim->get_component<WaterSimComponent>(), water_mat );
    auto mat = plane->add_component<MaterialComponent>( water_mat );

    get_ecs()
        .observer( "WaterSim_Material_Setter" )
        .event( EcsCoreEvent::OnSet )
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

    plane->add_component<MaterialRenderComponent>();
    water_tiles.push_back( plane );
}

void WorldScene::create_submerged_objects()
{
    auto& res = get_resource_loader();

    // Opaque boxes sitting below the surface, so depth-based water absorption is visible.
    // StandardSpatial multiplies albedo by the vertex color, so the mesh supplies white.
    constexpr Array<Vertex3D, 8> box_verts = {
        Vertex3D{ -1, -1, -1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0xFFFFFFFF },
        Vertex3D{ -1, 1, -1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0xFFFFFFFF },
        Vertex3D{ 1, 1, -1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0xFFFFFFFF },
        Vertex3D{ 1, -1, -1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0xFFFFFFFF },
        Vertex3D{ -1, -1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0xFFFFFFFF },
        Vertex3D{ -1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0xFFFFFFFF },
        Vertex3D{ 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0xFFFFFFFF },
        Vertex3D{ 1, -1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0xFFFFFFFF },
    };
    constexpr Array<uint16_t, 36> box_indices = { 2, 0, 1, 2, 3, 0, 4, 6, 5, 4, 7, 6, 0, 7, 4, 0, 3, 7,
                                                  1, 0, 4, 1, 4, 5, 1, 5, 2, 5, 6, 2, 3, 6, 7, 3, 2, 6 };

    auto box = Ref<Mesh>::create(
        MeshDesc{
            .vertices = DynamicArray<std::byte>(
                reinterpret_cast<std::byte const*>( box_verts.data() ),
                reinterpret_cast<std::byte const*>( box_verts.data() + box_verts.size() )
            ),
            .indices       = DynamicArray<uint16_t>( box_indices.data(), box_indices.data() + box_indices.size() ),
            .vertex_stride = sizeof( Vertex3D )
        }
    );
    auto box_rid = res.add( box );
    auto shader  = res.load( "shaders/materials/standard_spatial.slang" );

    struct Placement {
        Vec3f position;
        Vec3f scale;
        Vec4f color;
    };
    const Placement placements[] = {
        { Vec3f( 6.0f, -4.0f, 4.0f ), Vec3f( 3.0f, 3.0f, 3.0f ), Vec4f( 0.85f, 0.35f, 0.20f, 1.0f ) },
        { Vec3f( -8.0f, -6.0f, -2.0f ), Vec3f( 5.0f, 5.0f, 5.0f ), Vec4f( 0.20f, 0.55f, 0.85f, 1.0f ) },
        { Vec3f( 0.0f, -2.5f, 12.0f ), Vec3f( 4.0f, 2.0f, 4.0f ), Vec4f( 0.30f, 0.75f, 0.35f, 1.0f ) },
    };

    for (const auto& p : placements) {
        auto node = root()->create_child( "Submerged" );
        node->add_component<Transform3DComponent>(
            Transform3DComponent{ .Translation = p.position, .Rotation = Quaternion::identity(), .Scale = p.scale }
        );
        node->add_component<HasResourceTag>();
        node->add_component<MeshComponent>( MeshComponent{ .Source = box_rid } );
        node->add_component<MeshRenderComponent>();

        MaterialComponent mat;
        mat.Shader = shader;
        mat.set_params( "albedoColor", p.color );
        node->add_component<MaterialComponent>( mat );
        node->add_component<MaterialRenderComponent>();
    }
}

} // namespace sea
