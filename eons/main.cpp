#include "pch.h"

#include <ncore.hpp>

#include <microcosmos/MicrocosmModule.h>

class EonsApplication : public ncore::Application {
public:
    EonsApplication() :
        Application({"Eons",
                     ncore::AppVersion{.Major = GAME_VERSION_MAJOR,
                                       .Minor = GAME_VERSION_MINOR,
                                       .Patch = GAME_VERSION_PATCH,
                                       .Identifier = GAME_VERSION_IDENTIFIER},
                     "eons.ini"}) {}

    void on_world_init(ncore::IGameWorld &world) override {
        auto &scene = static_cast<ncore::Scene &>(world);
        scene.get_root_node()->create_child("Player");
    }
};

int main(int argc, char *argv[]) {
    EonsApplication app;
    app.init();
    app.run();
    app.finish();
    return 0;
}
