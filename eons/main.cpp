#include "pch.h"

#include <ncore.h>

#include <microcosmos/MicrocosmModule.h>

int main(int argc, char *argv[]) {
    auto title = std::format("EONS v{}.{}.{}-{}", GAME_VERSION_MAJOR, GAME_VERSION_MINOR, GAME_VERSION_PATCH,
                             GAME_VERSION_IDENTIFIER);
    ncore::GameLoop engine(title);

    auto world = ncore::EcsWorld();
    world.load<ncore::EcsEngineModule>();
    world.load<MicrocosmModule>();

    return (int) engine.run(&world);
}
