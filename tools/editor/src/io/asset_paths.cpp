#include "asset_paths.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#include <ncore/application.h>
#include <ncore/resources/resource.h>
#include <ncore/runtime/resources/resource_loader.h>

namespace nc::editor {

namespace {

constexpr int kMaxRootProbe = 8;

String path_string( const std::filesystem::path& p )
{
    return String( p.string().c_str() );
}

// Dev-build fallback: walk up from the working directory until a repository
// layout is found, then use its asset dirs in pipeline order.
DynamicArray<String> discover_source_dirs()
{
    std::error_code ec;
    auto dir = std::filesystem::current_path( ec );
    if (ec)
        return {};

    for (int i = 0; i <= kMaxRootProbe; ++i) {
        if (std::filesystem::exists( dir / "ncore" / "assets", ec )) {
            DynamicArray<String> dirs;
            for (const char* name : { "ncore", "eons-game", "seaengine" }) {
                const auto candidate = dir / name / "assets";
                if (std::filesystem::exists( candidate, ec ))
                    dirs.push_back( path_string( candidate ) );
            }
            return dirs;
        }

        if (!dir.has_parent_path())
            break;
        dir = dir.parent_path();
    }
    return {};
}

DynamicArray<String> parse_dirs( StringView value )
{
    DynamicArray<String> dirs;
    size_t begin = 0;

    for (;;) {
        const auto sep = value.find( ';', begin );
        const auto end = sep == StringView::npos ? value.size() : sep;

        size_t first = begin;
        size_t last  = end;
        while (first < last && std::isspace( static_cast<unsigned char>( value[first] ) ))
            ++first;
        while (last > first && std::isspace( static_cast<unsigned char>( value[last - 1] ) ))
            --last;

        if (last > first) {
            std::error_code ec;
            std::filesystem::path p( std::string( value.data() + first, last - first ) );
            if (!p.is_absolute())
                p = std::filesystem::current_path( ec ) / p;
            dirs.push_back( path_string( p.lexically_normal() ) );
        }

        if (sep == StringView::npos)
            break;
        begin = sep + 1;
    }

    return dirs;
}

// Mirrors cmake/CompileAssets.cmake: later source dirs win, so the search path
// list is reversed and earlier entries take priority in Slang.
void build_search_paths( const DynamicArray<String>& dirs, DynamicArray<String>& out )
{
    out.clear();
    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
        for (const char* sub : { "shaders", "shaders/core", "shaders/materials" }) {
            const auto p = std::filesystem::path( it->c_str() ) / sub;
            std::error_code ec;
            if (std::filesystem::exists( p, ec ))
                out.push_back( path_string( p ) );
        }
    }
}

bool read_text_file( const std::filesystem::path& p, std::string& out )
{
    std::ifstream in( p, std::ios::binary );
    if (!in.is_open())
        return false;
    out.assign( std::istreambuf_iterator<char>( in ), std::istreambuf_iterator<char>() );
    return true;
}

} // namespace

AssetPaths discover_asset_paths( const AppContext& app, const ResourceLoader& loader )
{
    AssetPaths paths;

    paths.source_dirs = discover_source_dirs();
    if (const String configured = app.config.get( "assets.SourceDirs" ); !configured.empty())
        paths.source_dirs = parse_dirs( configured );

    build_search_paths( paths.source_dirs, paths.search_paths );

    paths.asset_root = String( loader.get_asset_root() );

    std::error_code ec;
    paths.manifest_path = path_string( std::filesystem::current_path( ec ) / ".editor" / "asset_manifest" );

    return paths;
}

bool find_source( StringView asset_path, const AssetPaths& paths, String& out )
{
    std::error_code ec;
    for (auto it = paths.source_dirs.rbegin(); it != paths.source_dirs.rend(); ++it) {
        const auto candidate =
            std::filesystem::path( it->c_str() ) / std::string( asset_path.data(), asset_path.size() );
        if (std::filesystem::exists( candidate, ec )) {
            out = path_string( candidate );
            return true;
        }
    }
    return false;
}

bool resolve_include(
    StringView include, const std::filesystem::path& includer_dir, const AssetPaths& paths,
    std::filesystem::path& out )
{
    std::error_code ec;
    const std::filesystem::path rel( std::string( include.data(), include.size() ) );

    const auto local = includer_dir / rel;
    if (std::filesystem::exists( local, ec )) {
        out = local;
        return true;
    }

    for (const auto& dir : paths.search_paths) {
        const std::filesystem::path candidate = std::filesystem::path( dir.c_str() ) / rel;
        if (std::filesystem::exists( candidate, ec )) {
            out = candidate;
            return true;
        }
    }
    return false;
}

uint64_t fnv1a( StringView text, uint64_t seed )
{
    constexpr uint64_t kPrime = 1099511628211ull;
    for (unsigned char c : text) {
        seed ^= static_cast<uint64_t>( c );
        seed *= kPrime;
    }
    return seed;
}

uint64_t hash_file( const std::filesystem::path& p )
{
    std::ifstream in( p, std::ios::binary );
    if (!in.is_open())
        return 0;

    constexpr uint64_t kPrime = 1099511628211ull;

    uint64_t hash = 1469598103934665603ull;
    char buffer[64 * 1024];
    while (in.read( buffer, sizeof( buffer ) ) || in.gcount() > 0) {
        const auto count = in.gcount();
        for (std::streamsize i = 0; i < count; ++i) {
            hash ^= static_cast<uint64_t>( static_cast<unsigned char>( buffer[i] ) );
            hash *= kPrime;
        }
    }
    return hash;
}

uint64_t hash_source_tree( const std::filesystem::path& source, const AssetPaths& paths )
{
    std::set<std::string> visited;
    std::vector<std::string> pending{ source.string() };
    uint64_t hash = 1469598103934665603ull;

    while (!pending.empty()) {
        const std::string current = pending.back();
        pending.pop_back();

        std::error_code ec;
        const auto path = std::filesystem::path( current );
        const auto canonical = std::filesystem::weakly_canonical( path, ec ).string();
        if (!visited.insert( canonical ).second)
            continue;

        std::string text;
        if (!read_text_file( path, text ))
            continue;

        hash = fnv1a( canonical, hash );
        hash = fnv1a( text, hash );

        size_t pos = 0;
        while (( pos = text.find( "#include", pos )) != std::string::npos) {
            pos += 8;
            while (pos < text.size() && ( text[pos] == ' ' || text[pos] == '\t' ))
                ++pos;
            if (pos >= text.size() || ( text[pos] != '"' && text[pos] != '<' ))
                continue;

            const char terminator = text[pos] == '"' ? '"' : '>';
            const auto end        = text.find( terminator, pos + 1 );
            if (end == std::string::npos)
                break;

            const std::string include = text.substr( pos + 1, end - pos - 1 );
            pos                       = end + 1;

            std::filesystem::path resolved;
            if (resolve_include( include, path.parent_path(), paths, resolved ))
                pending.push_back( resolved.string() );
        }
    }

    return hash;
}

bool kind_has_includes( AssetKind kind )
{
    return kind == AssetKind::SHADER || kind == AssetKind::MATERIAL_SHADER;
}

bool read_format_id( const std::filesystem::path& p, uint32_t& out )
{
    std::ifstream in( p, std::ios::binary );
    if (!in.is_open())
        return false;
    in.read( reinterpret_cast<char*>( &out ), sizeof( out ) );
    return in.good();
}

AssetKind kind_from_format( uint32_t id )
{
    if (id == ResourceFormatID( "nimg" ).id)
        return AssetKind::IMAGE;
    if (id == ResourceFormatID( "ncbm" ).id)
        return AssetKind::CUBEMAP;
    if (id == ResourceFormatID( "nmsh" ).id)
        return AssetKind::MESH;
    if (id == ResourceFormatID( "nmat" ).id)
        return AssetKind::MATERIAL_SHADER;
    if (id == ResourceFormatID( "nshd" ).id)
        return AssetKind::SHADER;
    if (id == ResourceFormatID( "naud" ).id)
        return AssetKind::AUDIO;
    if (id == ResourceFormatID( "nfnt" ).id)
        return AssetKind::FONT;
    return AssetKind::UNKNOWN;
}

StringView asset_kind_name( AssetKind kind )
{
    switch (kind) {
        case AssetKind::IMAGE:
            return "image";
        case AssetKind::CUBEMAP:
            return "cubemap";
        case AssetKind::MESH:
            return "mesh";
        case AssetKind::MATERIAL_SHADER:
            return "material";
        case AssetKind::SHADER:
            return "shader";
        case AssetKind::AUDIO:
            return "audio";
        case AssetKind::FONT:
            return "font";
        default:
            return "unknown";
    }
}

bool asset_kind_can_import( AssetKind kind )
{
    switch (kind) {
        case AssetKind::IMAGE:
        case AssetKind::CUBEMAP:
        case AssetKind::MATERIAL_SHADER:
        case AssetKind::SHADER:
        case AssetKind::AUDIO:
        case AssetKind::FONT:
            return true;
        default:
            return false;
    }
}

bool asset_path_matches( StringView path, const char* filter )
{
    if (!filter || !filter[0])
        return true;

    const size_t needle_len = std::strlen( filter );
    if (path.size() < needle_len)
        return false;

    auto lower = []( char c ) { return static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) ); };

    for (size_t i = 0; i + needle_len <= path.size(); ++i) {
        size_t j = 0;
        for (; j < needle_len; ++j) {
            if (lower( path[i + j] ) != lower( filter[j] ))
                break;
        }
        if (j == needle_len)
            return true;
    }
    return false;
}

} // namespace nc::editor
