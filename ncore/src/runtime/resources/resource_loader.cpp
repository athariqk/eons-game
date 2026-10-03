#include <filesystem>

#include <ncore/resources/resource.h>
#include <ncore/resources/resource_archive.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/utils/log.h>
#include <ncore/utils/math.h>

namespace nc {

RID ResourceLoader::load( const String& path, bool skip_cache )
{
    // Compiled resources live next to their source files as "<path>.bin".
    // Callers keep asking for the source path; raw originals are never staged
    // into the runtime tree, so every asset — fonts included — loads from here.
    auto fs_path = std::filesystem::current_path() / "assets" / ( path + ".bin" );

    std::error_code ec;
    if (!std::filesystem::exists( fs_path, ec )) {
        if (ec) {
            NC_LOG_ERROR_C( log::IO, "OS error evaluating path: {}", ec.message() );
        } else {
            NC_LOG_ERROR_C( log::IO, "Requested resource does not exist on path: {}", fs_path.string() );
        }
        return RID();
    }

    auto cached     = path_map.find( path );
    bool has_cached = cached != path_map.end();
    if (!skip_cache && has_cached) {
        NC_LOG_DEBUG_C( log::IO, "load: cache HIT, RID={} filepath={}", cached->second.value, cached->first );
        auto ref = storage.get( cached->second );
        NC_VERIFY( ref );
        LoadEvent e;
        e.Handle   = cached->second;
        e.FormatId = ( *ref )->get_format_id();
        events.push( e );
        return cached->second;
    }

    // Deserialize from binary file.
    ResourceArchive archive;
    auto resource = archive.deserialize( fs_path.string().c_str() );
    if (!resource) {
        NC_LOG_ERROR_C( log::IO, "Failed to load resource from '{}'", path );
        return RID();
    }

    resource->filepath = path;

    auto handle   = storage.acquire();
    auto entry    = storage.get( handle );
    *entry        = resource;
    resource->rid = handle;

    path_map[path] = handle;

    NC_LOG_DEBUG_C(
        log::IO, "Loaded {} from '{}', RID={} ({} KB)", resource->get_class_name(), path, handle.value,
        math::bytes_to_kb( resource->get_size_bytes() )
    );

    LoadEvent e;
    e.Handle   = handle;
    e.FormatId = resource->get_format_id();
    events.push( e );

    return handle;
}

void ResourceLoader::unload_resource( RID rid )
{
    if (!rid.is_valid())
        return;

    auto entry = storage.get( rid );
    if (!entry)
        return;

    if (!( *entry )->filepath.empty())
        path_map.erase( ( *entry )->filepath );

    storage.release( rid );
}

void ResourceLoader::unload_all()
{
    storage.release_all();
    path_map.clear();
}

RID ResourceLoader::add( Ref<IResource> res )
{
    if (storage.contains( res->rid )) {
        // already added.
        return res->rid;
    }

    auto handle = storage.acquire();
    auto entry  = storage.get( handle );
    *entry      = res;
    res->rid    = handle;

    LoadEvent e;
    e.Handle   = handle;
    e.FormatId = res->get_format_id();
    events.push( e );

    return handle;
}

Ref<IResource> ResourceLoader::get( RID rid )
{
    auto entry = storage.get( rid );
    if (!entry)
        return nullptr;
    return *entry;
}

const ResourceLoader::Event* ResourceLoader::peek_event() const
{
    return events.peek();
}

bool ResourceLoader::poll_event( ResourceLoader::Event* event )
{
    auto removed = events.pop();
    if (removed) {
        *event = *removed;
    }
    return removed;
}

} // namespace nc
