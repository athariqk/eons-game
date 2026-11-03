// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <variant>

#include "resource.h"

namespace nc {

class Scene;

class NCAPI ResourceLoader : public Object {
    NCLASS( ResourceLoader, Object )

public:
    struct LoadEvent {
        RID handle;
        ResourceFormatID format_id;
        RID replaced = 0; // previous handle when this load was a forced reload.
    };

    using Event = std::variant<LoadEvent>;

    ResourceLoader( Scene& p_scene );

    /**
     * @brief Get the absolute path of the runtime `assets/` directory.
     */
    const char* get_asset_root() const NC_LIFETIME_BOUND;

    /**
     * @brief Loads a resource format from disk at given path and
     * caches the result in memory by filepath. Standard stuff.
     *
     * Blocks execution of the calling thread until load completes.
     *
     * @param p_path The relative path from the `assets` folder, NOT absolute path.
     * @param p_skip_cache Force re-loading from disk rather than from cache. Use this
     * for hot-reloading.
     *
     * @return Stable opaque handle as RID. Use get(RID) to access it.
     */
    RID load( const String& p_path, bool p_skip_cache = false );

    /**
     * @brief Inverse of load(). Returns the asset path a handle was loaded from.
     * @return Empty view for handles that were not loaded from disk.
     */
    StringView get_path_of( RID p_rid ) const NC_LIFETIME_BOUND;

    /**
     * @brief Resolve a path to its cached RID without emitting a LoadEvent.
     * @return The cached handle, or an empty RID if absent.
     */
    RID find( StringView p_path ) const;

    /**
     * @brief Reverse lookup: the cache handle a live resource was loaded as.
     *
     * Used by ResourceArchive to persist a Ref<T> whose resource has no asset
     * path (created in memory). The RID is process-local and only resolves
     * while this loader is alive.
     * @return The cached handle, or an empty RID for resources this loader did
     *         not create (or after they were unloaded).
     */
    RID find_handle( const IResource* p_resource ) const;

    void unload_resource( RID p_rid );
    void unload_all();

    /**
     * @brief Resolve loaded IResource instance by RID.
     *
     * TODO: this function's existence is probably more justified once
     * we've got asynchronous loading in place.
     */
    Ref<IResource> get( RID p_rid );

    /**
     * @brief Loads a resource format from disk at given path and
     * caches the result in memory by filepath. Standard stuff.
     *
     * Blocks execution of the calling thread until load completes.
     *
     * This is the same as calling non-generic load(const String&, bool) function
     * and then calling get(RID) on the returned RID.
     *
     * @param p_path The relative path from the `assets` folder, NOT absolute path.
     * @param p_skip_cache Force re-loading from disk rather than from cache. Use this
     * for hot-reloading.
     *
     * @return Typed reference to the loaded resource or null if fail.
     */
    template<class T>
    Ref<T> load( const String& p_path, bool p_skip_cache = false )
    {
        RID handle = load( p_path, p_skip_cache );
        return get<T>( handle );
    }

    /**
     * @brief Resolve loaded IResource instance by RID.
     *
     * TODO: this function's existence is probably more justified once
     * we've got asynchronous loading in place.
     */
    template<typename T>
    Ref<T> get( RID p_rid )
    {
        auto entry = get( p_rid );
        if (!entry)
            return nullptr;
        return entry.as<T>();
    }

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
    bool poll_event( Event* p_event );

private:
    HashMap<String, RID> path_map;
    HashMap<RID, String> rid_paths;
    // Reverse of rid_paths for resources without a file backing (filepath is
    // the "generated" sentinel). Only Ref<T> persistence needs this.
    HashMap<const IResource*, RID> handles;
    // Paths whose load is currently in flight. A Ref<T> chain that points back
    // at one of these must not recurse; the dependent ref stays empty.
    HashSet<String> loading;
    RIDPool<Ref<IResource>> storage;
    RingBuffer<Event> events;
    String asset_root;
};

} // namespace nc
