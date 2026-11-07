#define NC_LOG_CHANNEL_NAME "SEA"

#include <ncore/application.h>

// Global operator new/delete overrides for the SeaEngine executable.
// Must be included in exactly one translation unit in this module.
// memalloc/memfree resolve to DLL imports from ncore.dll, ensuring both
// modules share the same mimalloc heap.
#include <ncore/core/alloc_overrides.h>

#include "src/main_scene.h"

#define SEA_VERSION_MAJOR      0
#define SEA_VERSION_MINOR      1
#define SEA_VERSION_PATCH      0
#define SEA_VERSION_IDENTIFIER "pre-alpha"

class SeaEngineApp : public nc::Application {
public:
    SeaEngineApp( const nc::AppDesc& p_desc ) : nc::Application( p_desc ) {}

    nc::Ptr<nc::IGameWorld> create_world() override
    {
        return std::make_unique<sea::MainScene>();
    }
};

int main( int argc, char* argv[] )
{
    nc::CommandLineParser cmdline_parser;
    cmdline_parser.add_optional_bool( "editor", "Launch in editor/authoring mode (default: Explore Mode)", false );

    if (!cmdline_parser.parse( argc, argv )) {
        cmdline_parser.print_help( "SeaEngine" );
        NC_LOG_ERROR( "Command-line parse error: {}", cmdline_parser.get_error_message() );
        return 1;
    }

#if defined( DEBUG )
    cmdline_parser.set_bool( "editor", true );
#endif

    NC_LOG_INFO( "SeaEngine starting in {} mode", cmdline_parser.get_bool( "editor" ) ? "EDITOR" : "EXPLORE" );

    SeaEngineApp app(
        nc::AppDesc{
            .name = "SeaEngine",
            .version =
                nc::AppVersion{
                    .major      = SEA_VERSION_MAJOR,
                    .minor      = SEA_VERSION_MINOR,
                    .patch      = SEA_VERSION_PATCH,
                    .identifier = SEA_VERSION_IDENTIFIER
                },
            .config_file  = "seaengine.ini",
            .cmdline_args = &cmdline_parser
        }
    );

    if (app.init() != nc::Error::OK) {
        return 1;
    }
    app.run();
    app.finish();
    return 0;
}
