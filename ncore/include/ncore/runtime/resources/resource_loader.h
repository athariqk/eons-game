#pragma once

#include <variant>

#include <ncore/resources/resource.h>

namespace nc {

class NCAPI ResourceLoader : public Object {
    NCLASS( ResourceLoader, Object )

public:
    struct LoadEvent {
        RID handle;
        ResourceFormatID format_id;
        RID replaced = 0; // previous handle when this load was a forced reload.
    };

    using Event = std::variant<LoadEvent>;

    ResourceLoader();

    /**
     * @brief Get the absolute path of the runtime `assets/` directory.
     */
    const char* get_asset_root() const [[clang::lifetimebound]];

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
    StringView path_of( RID p_rid ) const [[clang::lifetimebound]];

    /**
     * @brief Resolve a path to its cached RID without emitting a LoadEvent.
     * @return The cached handle (disk or virtual), or an empty RID if absent.
     */
    RID find( StringView p_path ) const;

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
     * @brief Insert a procedurally generated resource to storage under a logical path.
     * It can then have access to the whole resource system.
     * A LoadEvent will be generated.
     *
     * Binds `p_path` so `load(p_path)` and `path_of(rid)` round-trip exactly
     * like a disk asset, with no `.bin` file on disk. Re-registering an existing
     * path publishes a new handle; the previous resource stays in storage so
     * live references keep working until observers rebind via the LoadEvent.
     *
     * @return Its newly-assigned RID handle or existing one.
     */
    RID add( Ref<IResource> p_res, StringView p_path );

    /**
     * @brief The logical paths registered through add(res, path), in order.
     *
     * These are procedural assets: they have no `.bin` on disk, only a live
     * resource in storage. The editor indexes them from here.
     */
    Span<const String> get_virtual_paths() const
    {
        return Span<const String>( virtual_paths.data(), virtual_paths.size() );
    }

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
    HashMap<uint64_t, String> rid_paths;
    DynamicArray<String> virtual_paths; // logical paths bound via add(res, path)
    RIDPool<Ref<IResource>> storage;
    RingBuffer<Event> events;
    String asset_root;
};

} // namespace nc
