// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <filesystem>

#include <ncore/runtime/io/resource_archive.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/utils/log.h>
#include <ncore/utils/math.h>
#include <utils/profiler.h>

namespace nc {

ResourceLoader::ResourceLoader( Scene& p_scene )
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

    // Cache check first: every entry in path_map was loaded from disk.
    auto cached           = path_map.find( p_path );
    const bool has_cached = cached != path_map.end();
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

    // Compiled resources live next to their source files as "<path>.bin".
    // Callers keep asking for the source path; raw originals are never staged
    // into the runtime tree, so every asset — fonts included — loads from here.
    auto fs_path = std::filesystem::path( get_asset_root() ) / ( p_path + ".bin" );

    // Cycle guard: a resource whose Ref<T> field points back into the load that
    // is still in flight must not recurse. The dependent ref reads back empty.
    if (!loading.insert( p_path ).second) {
        NC_LOG_WARN_C( log::IO, "load: recursive load of '{}' ignored", p_path );
        return RID();
    }
    struct LoadingScope {
        HashSet<String>& set;
        const String& key;
        ~LoadingScope()
        {
            set.erase( key );
        }
    } loading_scope{ loading, p_path };

    std::error_code ec;
    if (!std::filesystem::exists( fs_path, ec )) {
        if (ec) {
            NC_LOG_ERROR_C( log::IO, "OS error evaluating path: {}", ec.message() );
        } else {
            NC_LOG_ERROR_C( log::IO, "Requested resource does not exist on path: {}", fs_path.string() );
        }
        return RID();
    }

    // Force-reload: remember the handle being replaced so observers can rebind.
    const RID replaced = has_cached ? cached->second : RID();

    // Deserialize from binary file, resolving any Ref<T> fields through this
    // loader (and its cycle guard).
    Ref<IResource> resource = ResourceArchive::deserialize( fs_path.string().c_str(), this );
    NC_FAIL_MSG_RETVAL( resource, RID(), std::format( "Failed to load resource from '{}'", p_path ).c_str() );

    resource->filepath = p_path;

    auto handle = storage.acquire();
    auto entry  = storage.get( handle );
    *entry      = resource;

    path_map[p_path]        = handle;
    rid_paths[handle.value] = p_path;
    handles[resource.get()] = handle;

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

StringView ResourceLoader::get_path_of( RID p_rid ) const
{
    auto it = rid_paths.find( p_rid.value );
    if (it == rid_paths.end())
        return StringView();
    return StringView( it->second );
}

RID ResourceLoader::find( StringView p_path ) const
{
    const auto it = path_map.find( String( p_path.data(), p_path.size() ) );
    return it == path_map.end() ? RID() : it->second;
}

RID ResourceLoader::find_handle( const IResource* p_resource ) const
{
    if (!p_resource)
        return RID();
    const auto it = handles.find( p_resource );
    return it == handles.end() ? RID() : it->second;
}

void ResourceLoader::unload_resource( RID p_rid )
{
    if (!p_rid.is_valid())
        return;

    auto entry = storage.get( p_rid );
    if (!entry)
        return;

    const String filepath = ( *entry )->filepath;
    if (!filepath.empty())
        path_map.erase( filepath );
    rid_paths.erase( p_rid.value );
    handles.erase( ( *entry ).get() );

    storage.release( p_rid );
}

void ResourceLoader::unload_all()
{
    storage.release_all();
    path_map.clear();
    rid_paths.clear();
    handles.clear();
    loading.clear();
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
