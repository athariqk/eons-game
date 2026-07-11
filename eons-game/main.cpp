#include <ncore.hpp>

#include <microcosmos/MicrocosmModule.h>

#include "pch.h"

class EonsGame : public nc::Application {
public:
    EonsGame() :
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

    void on_world_init( nc::IGameWorld& world ) override
    {
        Application::on_world_init( world );
        auto& scene = static_cast<nc::Scene&>( world );
        scene.get_root_node()->create_child( "Player" );
    }
};

int main( int argc, char* argv[] )
{
    ( void ) argc;
    ( void ) argv;
    EonsGame game;
    game.init();
    game.run();
    game.finish();
    return 0;
}
