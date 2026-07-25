// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <array>
#include <memory>
#include <string>
#include <string_view>

#include <ncore/core/collection.h>
#include <ncore/core/object.h>
#include <ncore/core/rid.h>
#include <ncore/modules/io/resource_importer.h>
#include <ncore/modules/module.h>
#include <ncore/resources/resource.h>

namespace nc {

/**
 * @brief ResourceManager handles resource loading from disk
 * (and seems to be only just that at the moment...).
 *
 * TODO: asynchronous I/O, streaming, asset compilation
 */
class ResourceManager : public IModule {
    NCLASS( ResourceManager, IModule )

    static constexpr int MAX_IMPORTERS = 64;

public:
    ResourceManager();

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    void register_importer( std::unique_ptr<IResourceImporter>&& importer );

    template<class T>
    Ref<T> load( const std::string_view path )
    {
        RID handle = load_resource( path );
        return get_resource<T>( handle );
    }

    RID load_resource( const std::string_view path );
    void unload_resource( RID rid );
    void unload_all();

    template<std::derived_from<IResourceImporter> T, typename... TArgs>
    void register_importer( TArgs&&... args )
    {
        register_importer( std::make_unique<T>( std::forward<TArgs>( args )... ) );
    }

    template<typename T>
    Ref<T> get_resource( RID rid )
    {
        auto entry = storage.get( rid );
        if (!entry)
            return nullptr;

        return entry->as<T>();
    }

    size_t get_resource_count() const
    {
        return storage.get_size();
    }

private:
    int num_importers = 0;
    std::array<std::unique_ptr<IResourceImporter>, MAX_IMPORTERS> importers;

    HashMap<std::string, RID> path_map;
    ResourcePool<Ref<IResource>> storage;
};

} // namespace nc
