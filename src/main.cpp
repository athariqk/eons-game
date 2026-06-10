#include <Engine.h>

#include "MicrocosmWorld.h"

int main(int argc, char *argv[]) {
    ncore::Engine engine(std::format("EONS v{}.{}.{}-{}", GAME_VERSION_MAJOR, GAME_VERSION_MINOR, GAME_VERSION_PATCH,
                                    GAME_VERSION_IDENTIFIER));
    auto world = std::unique_ptr<MicrocosmWorld>(new MicrocosmWorld());
    engine.get_main_loop().change_world(std::move(world));
    return engine.run();
}
