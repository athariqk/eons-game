// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include <ncore/core/collection.h>
#include <ncore/core/object.h>
#include <ncore/core/reference.h>
#include <ncore/core/rid.h>
#include <ncore/resources/resource.h>
#include <ncore/services/service.h>

#include "resource_importer.h"

namespace nc {

/**
 * @brief ResourceService handles resource loading from disk
 * (and seems to be only just that at the moment...).
 *
 * TODO: asynchronous I/O, streaming, asset compilation
 */
class NCAPI ResourceService : public IService {
    NCLASS( ResourceService, IService )

    static constexpr int MAX_IMPORTERS = 64;

public:
    struct LoadEvent {
        RID Handle;
        ResourceFormatID FormatId;
    };

    using Event = std::variant<LoadEvent>;

    ResourceService();

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    void register_importer( std::unique_ptr<IResourceImporter>&& importer );

    /**
     * @brief Loads a resource format from disk at given path as a blocking operation.
     * Cache the result in memory by filepath. Standard stuff.
     *
     * @param path The relative path from the `assets` folder, NOT absolute path.
     * @param skip_cache Force re-loading from disk rather than from cache. Use this
     * for hot-reloading.
     *
     * @return Stable opaque handle as RID. Use get(RID) to access it.
     */
    RID load( const String& path, bool skip_cache = false );
    void unload_resource( RID rid );
    void unload_all();

    /**
     * @brief Insert a procedurally generated resource to storage.
     * It can then have access to the whole resource system.
     * A LoadEvent will be generated.
     *
     * @return Its newly-assigned RID handle or existing one.
     */
    RID add( Ref<IResource> res );

    /**
     * @brief Access to the underlying reference by RID.
     *
     * TODO: this function's existence is probably more justified once
     * we've got asynchronous loading in place.
     */
    Ref<IResource> get( RID rid );

    size_t get_resource_count() const
    {
        return storage.get_size();
    }

    /**
     * @brief Peek an event from queue, does not pull it.
     */
    const Event* peek_event() const;
    /**
     * @brief Pull (remove) events from queue into the event ptr param.
     * Systems can use this to pump_events for pending events.
     */
    bool poll_event( Event* event );

    template<std::derived_from<IResourceImporter> T, typename... TArgs>
    void register_importer( TArgs&&... args )
    {
        register_importer( std::make_unique<T>( std::forward<TArgs>( args )... ) );
    }

    /**
     * @brief Loads a resource format from disk at given path as a blocking operation.
     * Cache the result in memory by filepath. Standard stuff.
     *
     * This is the same as calling non-generic load(const String&, bool) function
     * and then calling get(RID) on the returned RID.
     *
     * @param path The relative path from the `assets` folder, NOT absolute path.
     * @param skip_cache Force re-loading from disk rather than from cache. Use this
     * for hot-reloading.
     *
     * @return Typed reference to the loaded resource.
     */
    template<class T>
    Ref<T> load( const String& path, bool skip_cache = false )
    {
        RID handle = load( path, skip_cache );
        return get<T>( handle );
    }

    /**
     * @brief Access to the underlying strong typed reference by RID.
     *
     * TODO: this function's existence is probably more justified once
     * we've got asynchronous loading in place.
     */
    template<typename T>
    Ref<T> get( RID rid )
    {
        auto entry = get( rid );
        if (!entry)
            return nullptr;
        return entry.as<T>();
    }

private:
    int num_importers = 0;
    std::array<std::unique_ptr<IResourceImporter>, MAX_IMPORTERS> importers;

    HashMap<String, RID> path_map;
    RIDPool<Ref<IResource>> storage;
    RingBuffer<Event> events;
};

} // namespace nc
