// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <filesystem>

#include <backends/sdl/sdl_audio_loader.h>
#include <backends/stb/stb_image_loader.h>

#include <ncore/resources/resource.h>
#include <ncore/services/io/resource_importer.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/utils/log.h>

#include <services/io/importers/material_template_importer.h>
#include <services/io/importers/slang_importer.h>

namespace nc {

ResourceService::ResourceService()
{
    for (auto& importer : importers)
        importer.reset();
}

Error ResourceService::init( ConfFile& cfg_file )
{
    register_importer<SDLAudioLoader>();
    register_importer<StbImageLoader>();
    register_importer<SlangImporter>();
    register_importer<MaterialImporter>();
    return Error::OK;
}

void ResourceService::shutdown()
{
    unload_all();
}

void ResourceService::register_importer( std::unique_ptr<IResourceImporter>&& importer )
{
    NC_FAIL_MSG_RET( num_importers < MAX_IMPORTERS, "Reached number of max importers, won't register" );
    importers[num_importers++] = std::move( importer );
}

static String get_extension( const StringView path )
{
    auto dot_pos = path.rfind( '.' );
    if (dot_pos == StringView::npos)
        return {};
    String ext( path.substr( dot_pos ) );
    std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower );
    return ext;
}

RID ResourceService::load( const String& path, bool skip_cache )
{
    auto fs_path     = std::filesystem::current_path() / "assets" / path;
    auto fs_path_str = String( fs_path.string() );

    std::error_code ec;
    if (std::filesystem::exists( fs_path )) {
        // ok
    } else if (ec) {
        NC_LOG_ERROR_C( log::IO, "OS error evaluating path: {}", ec.message() );
        return RID();
    } else {
        NC_LOG_ERROR_C( log::IO, "Requested resource does not exist on path: {}", path );
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

    NC_LOG_DEBUG_C( log::IO, "Importing resource from path: {}", path );

    String ext = get_extension( path );
    if (ext.empty()) {
        NC_LOG_ERROR_C( log::IO, "Cannot determine file extension for path: '{}'", path );
        return RID();
    }

    IResourceImporter* handler = nullptr;
    for (int i = 0; i < num_importers; ++i) {
        if (importers[i] && importers[i]->is_handling_extension( ext )) {
            handler = importers[i].get();
            break;
        }
    }

    if (!handler) {
        NC_LOG_ERROR_C( log::IO, "No importer registered for extension '{}'", ext );
        return RID();
    }

    IResourceImporter::Context ctx;
    ctx.load       = [&]( const String& path_ ) { return load( path_, skip_cache ); };
    ctx.get        = [&]( RID handle_ ) { return get( handle_ ); };
    ctx.skip_cache = skip_cache;

    auto result = handler->import( fs_path_str, ctx );
    if (!result) {
        NC_LOG_ERROR_C( log::IO, "Importer failed to load resource from '{}'", path );
        return RID();
    }
    result->filepath = path;

    result->rid = has_cached ? cached->second : storage.acquire();
    auto pooled = storage.get( result->rid );
    NC_VERIFY( pooled );

    *pooled        = result;
    path_map[path] = result->rid;
    NC_LOG_INFO_C(
        log::IO, "Imported {} from path '{}', RID={} ({} KB)", result->get_class_name(), path, result->rid.value,
        math::bytes_to_kb( result->get_size_bytes() )
    );

    LoadEvent e;
    e.Handle   = result->rid;
    e.FormatId = result->get_format_id();
    events.push( e );

    return result->rid;
}

void ResourceService::unload_resource( RID rid )
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

void ResourceService::unload_all()
{
    storage.release_all();
    path_map.clear();
}

RID ResourceService::add( Ref<IResource> res )
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

Ref<IResource> ResourceService::get( RID rid )
{
    auto entry = storage.get( rid );
    if (!entry)
        return nullptr;
    return *entry;
}

const ResourceService::Event* ResourceService::peek_event() const
{
    return events.peek();
}

bool ResourceService::poll_event( ResourceService::Event* event )
{
    auto removed = events.pop();
    if (removed) {
        *event = *removed;
    }
    return removed;
}

} // namespace nc
