#pragma once

#include <cstdint>

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>
#include <ncore/core/types.h>

namespace nc {
class RenderService;
class ResourceLoader;
} // namespace nc

namespace nc::editor {

enum class AssetKind : uint8_t {
    UNKNOWN = 0,
    IMAGE,
    CUBEMAP,
    MESH,
    MATERIAL_SHADER,
    SHADER,
    AUDIO,
    FONT,
    COUNT,
};

struct AssetEntry {
    String path;
    AssetKind kind     = AssetKind::UNKNOWN;
    uint64_t SizeBytes = 0;
    int64_t Mtime      = 0;
};

StringView asset_kind_name( AssetKind kind );

/**
 * @brief Whether a compiler exists for this kind (meshes have none yet).
 */
bool asset_kind_can_import( AssetKind kind );

/**
 * @brief Case-insensitive substring match used by the asset search boxes.
 */
bool asset_path_matches( StringView path, const char* filter );

class AssetIndex {
public:
    /**
     * @brief Rebuild the index by walking the runtime assets directory.
     */
    void scan( StringView asset_root );
    void release_gpu( RenderService& renderer );

    bool scanned() const
    {
        return is_scanned;
    }

    Span<const AssetEntry> entries() const
    {
        return Span<const AssetEntry>( assets.data(), assets.size() );
    }

    const AssetEntry* find( StringView path ) const [[clang::lifetimebound]];

    /**
     * @brief GPU texture for an image asset, created on first use and cached.
     *
     * `reload` re-uploads from disk into a new texture (the old one stays alive
     * so components that still reference it keep working).
     */
    RID texture_for( const String& path, ResourceLoader& loader, RenderService& renderer, bool reload = false );
    StringView path_for_texture( RID texture ) const [[clang::lifetimebound]];

private:
    DynamicArray<AssetEntry> assets;
    HashMap<String, RID> texture_by_path;
    HashMap<uint64_t, String> path_by_texture;
    String asset_root;
    bool is_scanned = false;
};

} // namespace nc::editor
