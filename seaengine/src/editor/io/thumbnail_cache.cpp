#include "thumbnail_cache.h"

#include <ncore/runtime/resources/image.h>
#include <ncore/runtime/scene.h>

namespace sea::editor {

void ThumbnailCache::begin_frame()
{
    thumb_uploads_this_frame = 0;
}

nc::RID ThumbnailCache::texture_for( const nc::String& path, nc::Scene& p_scene, bool reload )
{
    if (!reload) {
        if (auto it = texture_by_path.find( path ); it != texture_by_path.end())
            return it->second;
    }

    // Thumbnails are prepared lazily and budgeted: a big folder should not stall
    // the frame preparing every image at once.
    constexpr int MAX_UPLOADS_PER_FRAME = 4;
    if (!reload && thumb_uploads_this_frame >= MAX_UPLOADS_PER_FRAME)
        return nc::RID();
    ++thumb_uploads_this_frame;

    auto image = p_scene.get_resource_loader().load<nc::Image>( path, reload );
    if (!image)
        return nc::RID();

    // The image owns the GPU texture; this cache only remembers the handle.
    nc::RID texture = image->prepare( p_scene );
    if (!texture)
        return nc::RID();

    texture_lru.erase( std::remove( texture_lru.begin(), texture_lru.end(), path ), texture_lru.end() );
    texture_lru.push_back( path );

    texture_by_path[path] = texture;

    constexpr size_t MAX_CACHED_THUMBNAILS = 128;
    while (texture_lru.size() > MAX_CACHED_THUMBNAILS) {
        const nc::String oldest = texture_lru.front();
        texture_lru.erase( texture_lru.begin() );
        texture_by_path.erase( oldest );
    }

    return texture_by_path[path];
}

void ThumbnailCache::release_gpu()
{
    texture_by_path.clear();
    texture_lru.clear();
}

} // namespace sea::editor
