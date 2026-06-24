#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <ncore/utils/log.h>

int main(int argc, char *argv[]) {
    ncore::log::silence();
    return Catch::Session().run(argc, argv);
}
