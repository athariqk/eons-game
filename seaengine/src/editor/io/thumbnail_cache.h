#pragma once

#include <ncore/core/collection.h>
#include <ncore/structs/rid.h>

namespace nc {
class Scene;
} // namespace nc

namespace sea::editor {

/**
 * @brief Main-thread-only cache of editor thumbnail handles.
 *
 * Values are the prepared representation RID owned by the `Image` resource
 * (see `Image::prepare()`), keyed by asset path. The cache never destroys those
 * RIDs: the resource owns its GPU texture and drops it when it is released.
 */
class ThumbnailCache {
public:
    /** @brief Reset the per-frame upload budget. Call once per frame. */
    void begin_frame();

    /**
     * @brief Prepared texture handle for an image asset, prepared on first use.
     *
     * `reload` re-reads the image from disk and re-prepares it.
     */
    nc::RID texture_for( const nc::String& path, nc::Scene& p_scene, bool reload = false );

    /** @brief Drop every cached lookup. Resource-owned GPU data is untouched. */
    void release_gpu();

private:
    nc::HashMap<nc::String, nc::RID> texture_by_path;
    nc::DynamicArray<nc::String> texture_lru; // insertion order of cached lookups
    int thumb_uploads_this_frame = 0;
};

} // namespace sea::editor
