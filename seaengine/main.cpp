#define NC_LOG_CHANNEL_NAME "SEA"

#include <ncore/application.h>

// Global operator new/delete overrides for the SeaEngine executable.
// Must be included in exactly one translation unit in this module.
// memalloc/memfree resolve to DLL imports from ncore.dll, ensuring both
// modules share the same mimalloc heap.
#include <ncore/core/alloc_overrides.h>

#include "src/world_scene.h"

#define SEA_VERSION_MAJOR      0
#define SEA_VERSION_MINOR      1
#define SEA_VERSION_PATCH      0
#define SEA_VERSION_IDENTIFIER "pre-alpha"

class SeaEngineApp : public nc::Application {
public:
    SeaEngineApp() :
        Application(
            { "SeaEngine",
              nc::AppVersion{
                  .Major      = SEA_VERSION_MAJOR,
                  .Minor      = SEA_VERSION_MINOR,
                  .Patch      = SEA_VERSION_PATCH,
                  .Identifier = SEA_VERSION_IDENTIFIER
              },
              "seaengine.ini" }
        )
    {}

    std::unique_ptr<nc::IGameWorld> create_world() override
    {
        return std::make_unique<sea::WorldScene>();
    }
};

int main()
{
    SeaEngineApp app;
    app.init();
    app.run();
    app.finish();
    return 0;
}
