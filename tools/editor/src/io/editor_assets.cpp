#include "editor_assets.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <ncore/resources/resource.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/log.h>

namespace nc::editor {

namespace {

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

// The compiled file starts with ResourceArchive's DiskHeader, whose first field
// is the FourCC of the resource format.
bool read_format_id( const std::filesystem::path& p, uint32_t& out )
{
    std::ifstream in( p, std::ios::binary );
    if (!in.is_open())
        return false;
    in.read( reinterpret_cast<char*>( &out ), sizeof( out ) );
    return in.good();
}

} // namespace

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

void AssetIndex::scan( StringView p_asset_root )
{
    assets.clear();
    asset_root = String( p_asset_root.data(), p_asset_root.size() );
    is_scanned = true;

    std::error_code ec;
    const std::filesystem::path root( std::string( asset_root.data(), asset_root.size() ) );
    if (!std::filesystem::exists( root, ec ))
        return;

    for (auto it = std::filesystem::recursive_directory_iterator( root, ec );
         it != std::filesystem::recursive_directory_iterator(); it.increment( ec )) {
        if (ec)
            break;
        if (!it->is_regular_file( ec ) || it->path().extension() != ".bin")
            continue;

        auto rel = std::filesystem::relative( it->path(), root, ec );
        if (ec)
            continue;

        std::string rel_str = rel.string();
        rel_str.resize( rel_str.size() - 4 );
        std::replace( rel_str.begin(), rel_str.end(), '\\', '/' );

        AssetEntry entry;
        entry.path = String( rel_str.c_str(), rel_str.size() );

        uint32_t format_id = 0;
        entry.kind      = read_format_id( it->path(), format_id ) ? kind_from_format( format_id ) : AssetKind::UNKNOWN;
        entry.SizeBytes = it->file_size( ec );
        entry.Mtime     = it->last_write_time( ec ).time_since_epoch().count();

        assets.push_back( std::move( entry ) );
    }

    std::sort( assets.begin(), assets.end(), []( const AssetEntry& a, const AssetEntry& b ) {
        return a.path < b.path;
    } );

    NC_LOG_INFO_C( log::IO, "AssetIndex: {} assets under '{}'", assets.size(), root.string() );
}

void AssetIndex::release_gpu( RenderService& renderer )
{
    for (auto& [path, rid] : texture_by_path) {
        ( void ) path;
        if (rid)
            renderer.destroy_rid( rid );
    }
    texture_by_path.clear();
    path_by_texture.clear();
}

const AssetEntry* AssetIndex::find( StringView path ) const
{
    for (const auto& entry : assets) {
        if (StringView( entry.path ) == path)
            return &entry;
    }
    return nullptr;
}

RID AssetIndex::texture_for( const String& path, ResourceLoader& loader, RenderService& renderer, bool reload )
{
    if (!reload) {
        if (auto it = texture_by_path.find( path ); it != texture_by_path.end())
            return it->second;
    }

    auto image = loader.get<Image>( loader.load( path, reload ) );
    if (!image)
        return RID();

    RID texture = renderer.texture_2d_create( *image );
    if (!texture)
        return RID();

    texture_by_path[path]          = texture;
    path_by_texture[texture.value] = path;
    return RID( texture.value );
}

StringView AssetIndex::path_for_texture( RID texture ) const
{
    auto it = path_by_texture.find( texture.value );
    if (it == path_by_texture.end())
        return StringView();
    return StringView( it->second );
}

} // namespace nc::editor
