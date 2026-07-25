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
    register_importer<MaterialImporter>( this ); // TODO: implement better context passing (existing resources etc)
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

RID ResourceManager::load_resource( const std::string_view path )
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

    auto cached = path_map.find( fs_path_str );
    if (cached != path_map.end())
        return cached->second;

    NC_LOG_TRACE_C( log::IO, "Importing resource from path: {}", fs_path_str );

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

    auto resource = handler->import( fs_path_str );
    if (!resource) {
        NC_LOG_ERROR_C( log::IO, "Importer failed to load resource from '{}'", fs_path_str );
        return RID();
    }
    NC_LOG_INFO_C( log::IO, "Imported a {} from {}", resource->get_class_name(), fs_path_str );

    resource->filepath = fs_path_str;

    auto handle = storage.acquire();
    if (auto pooled = storage.get( handle ))
        *pooled = std::move( resource );

    path_map[fs_path_str] = handle;

    return handle;
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

} // namespace nc
