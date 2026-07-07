#include <ncore.hpp>

#include <microcosmos/MicrocosmModule.h>

#include "pch.h"

class EonsApplication : public nc::Application {
public:
    EonsApplication() :
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
        auto& scene = static_cast<nc::Scene&>( world );
        scene.get_root_node()->create_child( "Player" );
    }
};

int main( int argc, char* argv[] )
{
    ( void ) argc;
    ( void ) argv;
    EonsApplication app;
    app.init();
    app.run();
    app.finish();
    return 0;
}
