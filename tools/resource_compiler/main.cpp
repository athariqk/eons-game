#include <filesystem>

#include <ncore/core/types.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/utils/command_line_parser.h>
#include <ncore/utils/config.h>
#include <ncore/utils/log.h>

namespace {

/// Split a `|`-joined flag value into directories. `|` is the separator because
/// it cannot occur in a Windows path and CMake would treat `;` as a list separator.
nc::DynamicArray<nc::String> split_search_paths( nc::StringView p_joined )
{
    nc::DynamicArray<nc::String> out;
    size_t begin = 0;
    for (;;) {
        const auto sep = p_joined.find( '|', begin );
        const auto end = ( sep == nc::StringView::npos ) ? p_joined.size() : sep;
        if (end > begin)
            out.emplace_back( p_joined.data() + begin, end - begin );
        if (sep == nc::StringView::npos)
            break;
        begin = end + 1;
    }
    return out;
}

} // namespace

int main( int argc, char* argv[] )
{
    // Primitives, nc::String and nc::Object are only registered here; gen
    // static-init registers records, so without this every primitive field
    // TypeId lookup misses and serialize emits an empty body (or crashes in
    // VectorClass::visit on a null element TypeInfo).
    nc::rtti::TypeRegistry::initialize();

    nc::CommandLineParser cmd_line_parser;
    cmd_line_parser.add_required_string( "input", "raw asset input path (absolute, or relative to assets/)" );
    cmd_line_parser.add_required_string( "output", "compiled .bin output path (absolute, or relative to assets/)" );
    cmd_line_parser.add_optional_string(
        "search-paths", "directories to resolve #include targets against, joined with '|' (shader compilation only)"
    );

    if (!cmd_line_parser.parse( argc, argv )) {
        nc::log::print_error( "Failed to parse arguments: " + cmd_line_parser.get_error_message() );
        return 1;
    }

    nc::ConfFile cfg_file; // dummy

    nc::AssetService assets;
    assets.init( cfg_file );

    auto& input_path  = cmd_line_parser.get_string( "input" );
    auto& output_path = cmd_line_parser.get_string( "output" );

    nc::DynamicArray<nc::String> search_paths;
    if (cmd_line_parser.was_provided( "search-paths" ))
        search_paths = split_search_paths( cmd_line_parser.get_string( "search-paths" ) );

    nc::log::printf( "Compiling: {} → {}\n", input_path, output_path );

    if (assets.compile( input_path.data(), output_path.data(), search_paths ) == nc::Error::OK) {
        nc::log::printf( "OK: {}\n", output_path );
        return 0;
    } else {
        nc::log::print_error( "FAILED: " + nc::String( input_path ) );
        return 1;
    }
}
