// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <filesystem>

#include <backends/sdl/sdl_audio_loader.h>
#include <backends/sdl/sdl_image_loader.h>

#include <modules/io/importers/material_template_importer.h>
#include <modules/io/importers/slang_importer.h>
#include <ncore/modules/io/resource_importer.h>
#include <ncore/modules/io/resource_manager.h>
#include <ncore/resources/resource.h>
#include <ncore/utils/log.h>

namespace nc {

ResourceManager::ResourceManager()
{
    for (auto& importer : importers)
        importer.reset();
}

Error ResourceManager::init( ConfFile& cfg_file )
{
    register_importer<SDLAudioLoader>();
    register_importer<SDLImageLoader>();
    register_importer<SlangImporter>();
    register_importer<MaterialImporter>();
    return Error::OK;
}

void ResourceManager::shutdown()
{
    unload_all();
}

void ResourceManager::register_importer( std::unique_ptr<IResourceImporter>&& importer )
{
    NC_ASSERT_RET( num_importers < MAX_IMPORTERS, "Reached number of max importers, won't register" );
    importers[num_importers++] = std::move( importer );
}

static std::string get_extension( const std::string_view path )
{
    auto dot_pos = path.rfind( '.' );
    if (dot_pos == std::string_view::npos)
        return {};
    std::string ext( path.substr( dot_pos ) );
    std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower );
    return ext;
}

RID ResourceManager::load( const std::string_view path, bool skip_cache )
{
    auto fs_path     = std::filesystem::current_path() / "assets" / path;
    auto fs_path_str = fs_path.string();

    std::error_code ec;
    if (std::filesystem::exists( fs_path )) {
        // ok
    } else if (ec) {
        NC_LOG_ERROR_C( log::IO, "OS error evaluating path: {}", ec.message() );
        return RID();
    } else {
        NC_LOG_ERROR_C( log::IO, "Requested resource does not exist on path: {}", fs_path_str );
        return RID();
    }

    auto cached     = path_map.find( fs_path_str );
    bool has_cached = cached != path_map.end();
    if (!skip_cache && has_cached) {
        NC_LOG_DEBUG_C( log::IO, "load: cache HIT, RID={} filepath={}", cached->second.value, cached->first );
        auto ref = storage.get( cached->second );
        NC_VERIFY( ref );
        LoadEvent e;
        e.handle    = cached->second;
        e.format_id = ( *ref )->get_format_id();
        events.push( e );
        return cached->second;
    }

    NC_LOG_DEBUG_C( log::IO, "Importing resource from path: {}", fs_path_str );

    std::string ext = get_extension( path );
    if (ext.empty()) {
        NC_LOG_ERROR_C( log::IO, "Cannot determine file extension for path: '{}'", fs_path_str );
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
    ctx.load       = [&]( const std::string_view path_ ) { return load( path_, skip_cache ); };
    ctx.get        = [&]( RID handle_ ) { return get( handle_ ); };
    ctx.skip_cache = skip_cache;

    auto result = handler->import( fs_path_str, ctx );
    if (!result) {
        NC_LOG_ERROR_C( log::IO, "Importer failed to load resource from '{}'", fs_path_str );
        return RID();
    }
    result->filepath = fs_path_str;

    auto rid    = has_cached ? cached->second : storage.acquire();
    auto pooled = storage.get( rid );
    NC_VERIFY( pooled );

    *pooled               = result;
    path_map[fs_path_str] = rid;
    NC_LOG_INFO_C( log::IO, "Imported a {} from path {}. RID={}", result->get_class_name(), fs_path_str, rid.value );

    LoadEvent e;
    e.handle    = rid;
    e.format_id = result->get_format_id();
    events.push( e );

    return rid;
}

void ResourceManager::unload_resource( RID rid )
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

void ResourceManager::unload_all()
{
    storage.release_all();
    path_map.clear();
}

RID ResourceManager::add( const Ref<IResource>& res )
{
    auto handle = storage.acquire();
    auto entry  = storage.get( handle );
    *entry      = res;

    LoadEvent e;
    e.handle    = handle;
    e.format_id = res->get_format_id();
    events.push( e );

    return handle;
}

Ref<IResource> ResourceManager::get( RID rid )
{
    auto entry = storage.get( rid );
    if (!entry)
        return nullptr;
    return *entry;
}

const ResourceManager::Event* ResourceManager::peek_event() const
{
    return events.peek();
}

bool ResourceManager::poll_event( ResourceManager::Event* event )
{
    auto removed = events.pop();
    if (removed) {
        *event = *removed;
    }
    return removed;
}

} // namespace nc
