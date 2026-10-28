#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>

namespace nc {
class RenderService;
class ResourceLoader;
} // namespace nc

namespace nc::editor {

/**
 * @brief Main-thread-only cache of editor thumbnails (GPU textures).
 *
 * Textures are uploaded lazily and budgeted per frame, and evicted oldest-first
 * once the cache exceeds a bound. Editor thumbnails are disposable: components
 * reference textures through the ResourceLoader, not through this cache.
 */
class ThumbnailCache {
public:
    /** @brief Reset the per-frame upload budget. Call once per frame. */
    void begin_frame();

    /**
     * @brief GPU texture for an image asset, created on first use and cached.
     *
     * `reload` re-uploads from disk into a new texture (the old one stays alive
     * so components that still reference it keep working).
     */
    RID texture_for( const String& path, ResourceLoader& loader, RenderService& renderer, bool reload = false );

    StringView path_for_texture( RID texture ) const [[clang::lifetimebound]];

    void release_gpu( RenderService& renderer );

private:
    HashMap<String, RID> texture_by_path;
    HashMap<uint64_t, String> path_by_texture;
    DynamicArray<String> texture_lru; // insertion order of cached thumbnails
    int thumb_uploads_this_frame = 0;
};

} // namespace nc::editor
