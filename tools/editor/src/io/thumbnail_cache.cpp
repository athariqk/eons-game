#include "thumbnail_cache.h"

#include <algorithm>

#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/services/video/render_service.h>

namespace nc::editor {

void ThumbnailCache::begin_frame()
{
    thumb_uploads_this_frame = 0;
}

RID ThumbnailCache::texture_for( const String& path, ResourceLoader& loader, RenderService& renderer, bool reload )
{
    if (!reload) {
        if (auto it = texture_by_path.find( path ); it != texture_by_path.end())
            return it->second;
    }

    // Thumbnails are uploaded lazily and budgeted: a big folder should not stall
    // the frame uploading every image at once.
    constexpr int kMaxUploadsPerFrame = 4;
    if (!reload && thumb_uploads_this_frame >= kMaxUploadsPerFrame)
        return RID();
    ++thumb_uploads_this_frame;

    auto image = loader.get<Image>( loader.load( path, reload ) );
    if (!image)
        return RID();

    RID texture = renderer.texture_2d_create( *image );
    if (!texture)
        return RID();

    texture_lru.erase( std::remove( texture_lru.begin(), texture_lru.end(), path ), texture_lru.end() );
    texture_lru.push_back( path );

    texture_by_path[path]          = texture;
    path_by_texture[texture.value] = path;

    constexpr size_t kMaxCachedThumbnails = 128;
    while (texture_lru.size() > kMaxCachedThumbnails) {
        const String oldest = texture_lru.front();
        texture_lru.erase( texture_lru.begin() );

        auto it = texture_by_path.find( oldest );
        if (it == texture_by_path.end())
            continue;

        const RID stale = it->second;
        path_by_texture.erase( stale.value );
        texture_by_path.erase( it );
        if (stale)
            renderer.destroy_rid( stale );
    }

    return RID( texture.value );
}

StringView ThumbnailCache::path_for_texture( RID texture ) const
{
    auto it = path_by_texture.find( texture.value );
    if (it == path_by_texture.end())
        return StringView();
    return StringView( it->second );
}

void ThumbnailCache::release_gpu( RenderService& renderer )
{
    for (auto& [path, rid] : texture_by_path) {
        ( void ) path;
        if (rid)
            renderer.destroy_rid( rid );
    }
    texture_by_path.clear();
    path_by_texture.clear();
    texture_lru.clear();
}

} // namespace nc::editor
