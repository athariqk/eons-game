// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <array>
#include <memory>
#include <string>
#include <string_view>

#include <ncore/kernel/collection.h>
#include <ncore/kernel/object.h>
#include <ncore/kernel/rid.h>
#include <ncore/modules/module.h>
#include <ncore/modules/resource/resource.h>
#include <ncore/modules/resource/resource_importer.h>

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
    void finalize() override;

    void register_importer( std::unique_ptr<IResourceImporter>&& importer );

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

        if (!( *entry )->is_a<T>())
            return nullptr;

        return *entry;
    }

    size_t get_resource_count() const
    {
        return storage.get_size();
    }

private:
    int num_importers = 0;
    std::array<std::unique_ptr<IResourceImporter>, MAX_IMPORTERS> importers;

    UnorderedMap<std::string, RID> path_map;
    PagedResourcePool<Ref<IResource>> storage;
};

} // namespace nc
