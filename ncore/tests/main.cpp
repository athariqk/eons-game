#define CATCH_CONFIG_MAIN
#include <ncore/utils/log.h>

#include <catch2/catch_all.hpp>

int main(int argc, char* argv[])
{
    ncore::log::silence();
    return Catch::Session().run(argc, argv);
}
