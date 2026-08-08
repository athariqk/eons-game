#include <ncore.hpp>

#include <microcosmos/MicrocosmModule.h>

#include "pch.h"

#include <editor/ncore_editor.h>

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

        auto test_model = root()->create_child( "TestModel3D" );
        test_model->add_component<nc::Transform3DComponent>( nc::Transform3DComponent{
            nc::Vec3( 0, 0, -10 ), nc::Quaternion( 180, nc::Vec3::up() ), nc::Vec3( 1, 1, 1 )
        } );

        auto cube_mesh = test_model->create_child( "CubeMesh" );
        cube_mesh->add_component<nc::MeshComponent>( nc::MeshComponent{ mesh_rid, nc::RID(), 1 } );
        cube_mesh->add_component<nc::MaterialComponent>(
            nc::MaterialComponent{ res_svc->load( "engine/materials/pbr.material" ) }
        );
        cube_mesh->add_component<nc::HasResourceTag>();
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
