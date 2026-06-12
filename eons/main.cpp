#include <Engine.h>

#include "microcosmos/MicrocosmWorld.h"

int main(int argc, char *argv[]) {
    auto title = std::format("EONS v{}.{}.{}-{}", GAME_VERSION_MAJOR, GAME_VERSION_MINOR, GAME_VERSION_PATCH,
                             GAME_VERSION_IDENTIFIER);
    ncore::Engine engine(title);
    auto world = std::make_unique<MicrocosmWorld>();
    return engine.run(std::move(world));
}
