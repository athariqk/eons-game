#include "world_scene.h"

#include <editor/ncore_editor.h>
#include <ncore/application.h>
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
#include <ncore/services/input/input_service.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/equirect.h>

namespace sea {

void WorldScene::on_ready()
{
    nc::editor::register_editor_plugin( *this );

#if defined( DEBUG )
    get_ecs().system( "HotReload" ).in( nc::EcsSystemPhase::UPDATE ).run( []( nc::QueryContext& ctx ) {
        auto io = ctx.world().get_singleton<nc::IoServices>();

        if (io->Inputs->is_key_pressed( nc::Key::F5 )) {
            nc::log::print( "Hot-reloading" );
            io->Resources->load<nc::MaterialTemplate>( "shaders/skybox.slang", true );
            io->Resources->load<nc::MaterialTemplate>( "shaders/water.slang", true );
            io->Resources->load<nc::MaterialTemplate>( "materials/skybox.material", true );
            io->Resources->load<nc::MaterialTemplate>( "materials/water.material", true );
        }
    } );
#endif

    create_environment();
    create_water();

    auto main_camera = root()->create_child( "MainCamera" );
    main_camera->add_component<nc::ActiveCameraTag>();
    main_camera->add_component<nc::Transform3DComponent>(
        nc::Transform3DComponent{ nc::Vec3( 0, 0, 5 ), nc::Quaternion::identity(), nc::Vec3( 1, 1, 1 ) }
    );
    main_camera->add_component<nc::CameraComponent>();
    main_camera->add_component<nc::InputComponent>();
}

void WorldScene::create_environment()
{
    auto res = get_app_ctx()->Services.resolve<nc::ResourceService>();
    auto rd  = get_app_ctx()->Services.resolve<nc::RenderService>();

    constexpr nc::Array<nc::Vertex3D, 8> box_verts = {
        nc::Vertex3D{ -1.0f, -1.0f, -1.0f }, nc::Vertex3D{ -1.0f, 1.0f, -1.0f }, nc::Vertex3D{ 1.0f, 1.0f, -1.0f },
        nc::Vertex3D{ 1.0f, -1.0f, -1.0f },  nc::Vertex3D{ -1.0f, -1.0f, 1.0f }, nc::Vertex3D{ -1.0f, 1.0f, 1.0f },
        nc::Vertex3D{ 1.0f, 1.0f, 1.0f },    nc::Vertex3D{ 1.0f, -1.0f, 1.0f }
    };
    constexpr nc::Array<uint16_t, 36> box_indices = {
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

    auto skybox_mesh = nc::Ref<nc::Mesh>::create(
        nc::MeshDesc{
            .vertices = nc::DynamicArray<std::byte>(
                reinterpret_cast<std::byte const*>( box_verts.data() ),
                reinterpret_cast<std::byte const*>( box_verts.data() + box_verts.size() )
            ),
            .indices       = nc::DynamicArray<uint16_t>( box_indices.data(), box_indices.data() + box_indices.size() ),
            .vertex_stride = sizeof( nc::Vertex3D )
        }
    );
    auto skybox_mesh_rid = res->add( skybox_mesh );

    auto equirect = res->get<nc::Image>( res->load( "images/skybox.png" ) );
    auto faces    = nc::equirect_to_cube( *equirect, equirect->get_width() / 4 );

    const nc::Image* face_ptrs[6];
    for (int i = 0; i < 6; i++)
        face_ptrs[i] = faces[i].get();

    auto skybox_tex = rd->texture_cube_create( face_ptrs );

    nc::MaterialComponent skybox_mat;
    skybox_mat.Source = res->load( "materials/skybox.material" );
    skybox_mat.add_texture( skybox_tex );

    auto skybox = root()->create_child( "Skybox" );
    skybox->add_component<nc::HasResourceTag>();
    skybox->add_component<nc::MeshComponent>( nc::MeshComponent{ skybox_mesh_rid } );
    skybox->add_component<nc::MaterialComponent>( skybox_mat );
}

void WorldScene::create_water()
{
    auto res_svc = get_app_ctx()->Services.resolve<nc::ResourceService>();

    auto mesh     = nc::Ref<nc::PlaneMesh>::create( 16, 16 );
    auto mesh_rid = res_svc->add( mesh );
    auto plane    = root()->create_child( "WaterPlane" );
    plane->add_component<nc::Transform3DComponent>(
        nc::Transform3DComponent{ nc::Vec3( 0, -1, 0 ), nc::Quaternion::identity(), nc::Vec3( 5, 1, 5 ) }
    );
    auto plane_mesh = plane->create_child( "WaterMesh" );
    plane_mesh->add_component<nc::HasResourceTag>();
    plane_mesh->add_component<nc::MeshComponent>( nc::MeshComponent{ mesh_rid } );
    plane_mesh->add_component<nc::MaterialComponent>(
        nc::MaterialComponent{ res_svc->load( "materials/water.material" ) }
    );
}

} // namespace sea
