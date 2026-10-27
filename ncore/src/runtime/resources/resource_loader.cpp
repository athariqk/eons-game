#include <filesystem>

#include <ncore/resources/resource.h>
#include <ncore/resources/resource_archive.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/utils/log.h>
#include <ncore/utils/math.h>
#include <utils/profiler.h>

namespace nc {

ResourceLoader::ResourceLoader()
{
    asset_root = ( std::filesystem::current_path() / "assets" ).string();
}

const char* ResourceLoader::get_asset_root() const
{
    return asset_root.c_str();
}

RID ResourceLoader::load( const String& p_path, bool p_skip_cache )
{
    NC_ZONE_COLOR( "ResourceLoader::load", NC_COLOR_RESOURCE )
    // Compiled resources live next to their source files as "<path>.bin".
    // Callers keep asking for the source path; raw originals are never staged
    // into the runtime tree, so every asset — fonts included — loads from here.
    auto fs_path = std::filesystem::path( get_asset_root() ) / ( p_path + ".bin" );

    std::error_code ec;
    if (!std::filesystem::exists( fs_path, ec )) {
        if (ec) {
            NC_LOG_ERROR_C( log::IO, "OS error evaluating path: {}", ec.message() );
        } else {
            NC_LOG_ERROR_C( log::IO, "Requested resource does not exist on path: {}", fs_path.string() );
        }
        return RID();
    }

    auto cached     = path_map.find( p_path );
    bool has_cached = cached != path_map.end();
    if (!p_skip_cache && has_cached) {
        NC_LOG_DEBUG_C( log::IO, "load: cache HIT, RID={} filepath={}", cached->second.value, cached->first );
        auto ref = storage.get( cached->second );
        NC_VERIFY( ref );
        LoadEvent e;
        e.handle    = cached->second;
        e.format_id = ( *ref )->get_format_id();
        events.push( e );
        return cached->second;
    }

    // Force-reload: remember the handle being replaced so observers can rebind.
    const RID replaced = has_cached ? cached->second : RID();

    // Deserialize from binary file.
    ResourceArchive archive;
    auto resource = archive.deserialize( fs_path.string().c_str() );
    if (!resource) {
        NC_LOG_ERROR_C( log::IO, "Failed to load resource from '{}'", p_path );
        return RID();
    }

    resource->filepath = p_path;

    auto handle   = storage.acquire();
    auto entry    = storage.get( handle );
    *entry        = resource;
    resource->rid = handle;

    path_map[p_path]        = handle;
    rid_paths[handle.value] = p_path;

    NC_LOG_DEBUG_C(
        log::IO, "Loaded {} from '{}', RID={} ({} KB)", resource->get_class_name(), p_path, handle.value,
        math::bytes_to_kb( resource->get_size_bytes() )
    );

    LoadEvent e;
    e.handle    = handle;
    e.format_id = resource->get_format_id();
    e.replaced  = replaced;
    events.push( e );

    return handle;
}

StringView ResourceLoader::path_of( RID p_rid ) const
{
    auto it = rid_paths.find( p_rid.value );
    if (it == rid_paths.end())
        return StringView();
    return StringView( it->second );
}

void ResourceLoader::unload_resource( RID p_rid )
{
    if (!p_rid.is_valid())
        return;

    auto entry = storage.get( p_rid );
    if (!entry)
        return;

    if (!( *entry )->filepath.empty())
        path_map.erase( ( *entry )->filepath );
    rid_paths.erase( p_rid.value );

    storage.release( p_rid );
}

void ResourceLoader::unload_all()
{
    storage.release_all();
    path_map.clear();
    rid_paths.clear();
}

RID ResourceLoader::add( Ref<IResource> p_res )
{
    if (storage.contains( p_res->rid )) {
        // already added.
        return p_res->rid;
    }

    auto handle = storage.acquire();
    auto entry  = storage.get( handle );
    *entry      = p_res;
    p_res->rid  = handle;

    LoadEvent e;
    e.handle    = handle;
    e.format_id = p_res->get_format_id();
    events.push( e );

    return handle;
}

Ref<IResource> ResourceLoader::get( RID p_rid )
{
    auto entry = storage.get( p_rid );
    if (!entry)
        return nullptr;
    return *entry;
}

const ResourceLoader::Event* ResourceLoader::peek_event() const
{
    return events.peek();
}

bool ResourceLoader::poll_event( ResourceLoader::Event* p_event )
{
    auto removed = events.pop();
    if (removed) {
        *p_event = *removed;
    }
    return removed;
}

} // namespace nc
