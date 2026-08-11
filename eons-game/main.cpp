#include <ncore_editor.h>

#include <ncore.hpp>
#include <ncore/runtime/components/camera.h>
#include <ncore/runtime/components/input.h>

#include <microcosmos/MicrocosmModule.h>

#include "pch.h"

struct TestSpin {
    float rotation       = 0;
    bool switch_rot      = true;
    nc::Quaternion start = nc::Quaternion( 180, nc::Vec3::up() );
    nc::Quaternion end   = nc::Quaternion( 0, nc::Vec3::up() );
    NSTRUCT(
        TestSpin, NC_F( TestSpin, rotation ) NC_F( TestSpin, switch_rot ) NC_F( TestSpin, start ) NC_F( TestSpin, end )
    )
};

class TestScene : public nc::Scene {
public:
    TestScene( nc::AppDesc& p_app_desc, nc::ServiceRegistry& p_services ) : Scene( p_app_desc, p_services ) {}

    void on_ready() override
    {
#if defined( DEBUG )
        nc::editor::register_editor_plugin( *this );
#endif

        auto res_svc = services.resolve<nc::ResourceService>();

        // clang-format off
		nc::Array<nc::Vertex3D, 8> cube_verts = {
						//  px,    py,    pz,    nx,   ny,   nz,   tx,   ty,   tz,   tw,   u,    v,    u2,   v2,   color
			nc::Vertex3D{ -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF0000FF },
			nc::Vertex3D{ -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF00FF00 },
			nc::Vertex3D{  1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFF0000 },
			nc::Vertex3D{  1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			
			nc::Vertex3D{ -1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF00FFFF },
			nc::Vertex3D{ -1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFF00 },
			nc::Vertex3D{  1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFF00FF },
			nc::Vertex3D{  1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF333333 }
		};
		nc::Array<uint16_t, 36> cube_indices = {
			2,0,1, 2,3,0,
			4,6,5, 4,7,6,
			0,7,4, 0,3,7,
			1,0,4, 1,4,5,
			1,5,2, 5,6,2,
			3,6,7, 3,2,6
		};
		nc::Array<nc::Vertex3D, 4> plane_verts = {
						//  px,   py,    pz,    nx,   ny,   nz,   tx,   ty,   tz,   tw,   u,    v,    u2,   v2,   color
			nc::Vertex3D{ -1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
			nc::Vertex3D{ -1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0xFFFFFFFF },
			nc::Vertex3D{  1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0xFFFFFFFF },
			nc::Vertex3D{  1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF }
		};
		nc::Array<uint16_t, 6> plane_indices = {
			0, 2, 1,
			0, 3, 2
		};
        // clang-format on

        auto mesh = nc::Ref<nc::Mesh>::create(
            nc::MeshDesc{
                .vertices = nc::DynamicArray<std::byte>(
                    reinterpret_cast<std::byte const*>( cube_verts.data() ),
                    reinterpret_cast<std::byte const*>( cube_verts.data() + 8 )
                ),
                .indices       = nc::DynamicArray<uint16_t>( cube_indices.data(), cube_indices.data() + 36 ),
                .vertex_stride = sizeof( nc::Vertex3D )
            }
        );
        auto mesh_rid = res_svc->add( mesh );

        auto pmesh = nc::Ref<nc::Mesh>::create(
            nc::MeshDesc{
                .vertices = nc::DynamicArray<std::byte>(
                    reinterpret_cast<std::byte const*>( plane_verts.data() ),
                    reinterpret_cast<std::byte const*>( plane_verts.data() + 4 )
                ),
                .indices       = nc::DynamicArray<uint16_t>( plane_indices.data(), plane_indices.data() + 6 ),
                .vertex_stride = sizeof( nc::Vertex3D )
            }
        );
        auto pmesh_rid = res_svc->add( pmesh );

        auto test_model = root()->create_child( "TestModel3D" );
        test_model->add_component<nc::Transform3DComponent>( nc::Transform3DComponent{
            nc::Vec3( 0, 0, 0 ), nc::Quaternion( 180, nc::Vec3::up() ), nc::Vec3( 1, 1, 1 )
        } );
        test_model->add_component<TestSpin>();

        auto cube_mesh = test_model->create_child( "CubeMesh" );
        cube_mesh->add_component<nc::MeshComponent>( nc::MeshComponent{ mesh_rid, nc::RID(), 1 } );
        cube_mesh->add_component<nc::HasResourceTag>();
        cube_mesh->add_component<nc::MaterialComponent>(
            nc::MaterialComponent{ res_svc->load( "materials/world_instance.material" ) }
        );

        auto plane = root()->create_child( "Plane" );
        plane->add_component<nc::Transform3DComponent>(
            nc::Transform3DComponent{ nc::Vec3( 0, -5, 0 ), nc::Quaternion::identity(), nc::Vec3( 5, 1, 5 ) }
        );
        auto plane_mesh = plane->create_child( "PlaneMesh" );
        plane_mesh->add_component<nc::MeshComponent>( nc::MeshComponent{ pmesh_rid, nc::RID(), 1 } );
        plane_mesh->add_component<nc::HasResourceTag>();
        plane_mesh->add_component<nc::MaterialComponent>(
            nc::MaterialComponent{ res_svc->load( "materials/world_instance.material" ) }
        );

        auto main_camera = root()->create_child( "MainCamera" );
        main_camera->add_component<nc::Transform3DComponent>(
            nc::Transform3DComponent{ nc::Vec3( 0, 0, 5 ), nc::Quaternion::identity(), nc::Vec3( 1, 1, 1 ) }
        );
        main_camera->add_component<nc::CameraComponent>();
        main_camera->add_component<nc::ActiveCameraTag>();
        main_camera->add_component<nc::InputComponent>();

        add_system<nc::Transform2DComponent, TestSpin>(
            []( nc::Node& node, nc::Transform2DComponent& xform, TestSpin& spin, double delta_time ) {
                // auto xform = ctx.get_component<nc::Transform2DComponent>();
                // xform->angle += static_cast<float>( ctx.delta_time() ) * 3.5f;

                // auto draw_list = ImGui::GetForegroundDrawList();
                // auto center    = xform->get_world_center_point();
                // char tmps[512];
                // std::snprintf(
                //     tmps, 512, "Translation: X=%.3f Y=%.3f\nAngle: %.3f deg", static_cast<double>( xform->position.x
                //     ), static_cast<double>( xform->position.y ), static_cast<double>( xform->angle )
                //);
                // draw_list->AddText( ImVec2( xform->position.x, xform->position.y ), IM_COL32_BLACK, tmps );
                // draw_list->AddCircleFilled( ImVec2( center.x, center.y ), 6, IM_COL32( 255, 0, 0, 255 ) );
            },
            nc::EcsSystemPhase::UPDATE
        );

        add_system<nc::Transform3DComponent, TestSpin>(
            []( nc::Node& node, nc::Transform3DComponent& xform, TestSpin& spin, double delta_time ) {
                spin.rotation += static_cast<float>( delta_time );
                if (spin.rotation >= 1.0f) {
                    spin.rotation                = 0.0f;
                    spin.start                   = spin.end;
                    nc::Quaternion flip_180      = nc::Quaternion( 180, nc::Vec3::up() );
                    nc::Quaternion weird_swaying = nc::Quaternion( 30, nc::Vec3::forward() );
                    spin.end                     = spin.start * flip_180 * weird_swaying;
                }
                xform.rotation = nc::Quaternion::slerp( spin.start, spin.end, std::min( spin.rotation, 1.0f ) );
            },
            nc::EcsSystemPhase::UPDATE
        );
    }
};

class GameApplication : public nc::Application {
public:
    GameApplication() :
        Application(
            { "Eons",
              nc::AppVersion{
                  .Major      = GAME_VERSION_MAJOR,
                  .Minor      = GAME_VERSION_MINOR,
                  .Patch      = GAME_VERSION_PATCH,
                  .Identifier = GAME_VERSION_IDENTIFIER
              },
              "eons.ini" }
        )
    {}

    std::unique_ptr<nc::IGameWorld> create_world() override
    {
        return std::make_unique<TestScene>( app_desc, services );
    }
};

int main( int argc, char* argv[] )
{
    ( void ) argc;
    ( void ) argv;
    GameApplication game;
    game.init();
    game.run();
    game.finish();
    return 0;
}
