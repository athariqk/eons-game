#pragma once

#include <variant>

#include <ncore/resources/resource.h>

namespace nc {

class NCAPI ResourceLoader : public Object {
    NCLASS( ResourceLoader, Object )

public:
    struct LoadEvent {
        RID Handle;
        ResourceFormatID FormatId;
    };

    using Event = std::variant<LoadEvent>;

    /**
     * @brief Loads a resource format from disk at given path and
     * caches the result in memory by filepath. Standard stuff.
     *
     * Blocks execution of the calling thread until load completes.
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
     * @brief Resolve loaded IResource instance by RID.
     *
     * TODO: this function's existence is probably more justified once
     * we've got asynchronous loading in place.
     */
    Ref<IResource> get( RID rid );

    /**
     * @brief Insert a procedurally generated resource to storage.
     * It can then have access to the whole resource system.
     * A LoadEvent will be generated.
     *
     * @return Its newly-assigned RID handle or existing one.
     */
    RID add( Ref<IResource> res );

    /**
     * @brief Loads a resource format from disk at given path and
     * caches the result in memory by filepath. Standard stuff.
     *
     * Blocks execution of the calling thread until load completes.
     *
     * This is the same as calling non-generic load(const String&, bool) function
     * and then calling get(RID) on the returned RID.
     *
     * @param path The relative path from the `assets` folder, NOT absolute path.
     * @param skip_cache Force re-loading from disk rather than from cache. Use this
     * for hot-reloading.
     *
     * @return Typed reference to the loaded resource or null if fail.
     */
    template<class T>
    Ref<T> load( const String& path, bool skip_cache = false )
    {
        RID handle = load( path, skip_cache );
        return get<T>( handle );
    }

    /**
     * @brief Resolve loaded IResource instance by RID.
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

private:
    HashMap<String, RID> path_map;
    RIDPool<Ref<IResource>> storage;
    RingBuffer<Event> events;
};

} // namespace nc
