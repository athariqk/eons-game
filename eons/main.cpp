#include "pch.h"

#include <ncore.hpp>

#include <microcosmos/MicrocosmModule.h>

class EonsApplication : public ncore::Application {
public:
    EonsApplication() :
        Application("Eons",
                    ncore::AppVersion{.Major = GAME_VERSION_MAJOR,
                                      .Minor = GAME_VERSION_MINOR,
                                      .Patch = GAME_VERSION_PATCH,
                                      .Identifier = GAME_VERSION_IDENTIFIER},
                    "eons.ini") {}

    std::unique_ptr<ncore::IGameWorld> create_world() override {
        auto ecs_world = std::make_unique<ncore::EcsWorld>(get_event_bus(), services);
        ecs_world->load<ncore::EcsEngineModule>();
        ecs_world->load<MicrocosmModule>();
        return ecs_world;
    }
};

int main(int argc, char *argv[]) {
    EonsApplication app;
    app.init();
    app.run();
    app.finish();
    return 0;
}
