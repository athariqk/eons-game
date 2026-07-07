// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>

#include <ncore/modules/resource/resource.h>
#include <ncore/modules/resource/resource_importer.h>
#include <ncore/modules/resource/resource_manager.h>
#include <ncore/utils/log.h>

namespace nc {

ResourceManager::ResourceManager()
{
    for (auto& importer : importers)
        importer.reset();
}

Error ResourceManager::init()
{
    return Error::OK;
}

void ResourceManager::finalize()
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
    std::string path_str( path );

    auto cached = path_map.find( path_str );
    if (cached != path_map.end())
        return cached->second;

    std::string ext = get_extension( path );
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

    auto resource = handler->import( path );
    if (!resource) {
        NC_LOG_ERROR_C( log::IO, "Importer failed to load resource from '{}'", path );
        return RID();
    }

    resource->filepath = path_str;

    RID rid            = next_rid++;
    storage[rid]       = resource;
    path_map[path_str] = rid;

    return rid;
}

void ResourceManager::unload_resource( RID rid )
{
    if (!rid.is_valid())
        return;

    auto it = storage.find( rid );
    if (it == storage.end())
        return;

    auto& resource = it->second;
    if (!resource->filepath.empty())
        path_map.erase( resource->filepath );

    storage.erase( it );
}

void ResourceManager::unload_all()
{
    storage.clear();
    path_map.clear();
}

} // namespace nc
