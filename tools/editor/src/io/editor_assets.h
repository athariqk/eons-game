#pragma once

#include <cstdint>
#include <functional>

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>

#include <io/asset_paths.h>

namespace nc {
class RenderService;
class ResourceLoader;
} // namespace nc

namespace nc::editor {

class AssetImporter;

/**
 * @brief Where an asset comes from.
 */
enum class AssetOrigin : uint8_t {
    File = 0,   // source on disk -> compiled .bin
    Procedural, // registered generator or a loader virtual path
};

/**
 * @brief Current state of an asset, computed during scan and updated by the
 * import service / procedural generation.
 */
enum class AssetStatus : uint8_t {
    Ready = 0,     // compiled matches source (or procedural is up-to-date)
    Dirty,         // source/params newer than the last compile/generation
    MissingSource, // no raw source found (file assets)
    Compiling,     // reimport/generation job in flight
    Error,         // last compile/generation failed (see last_error)
};

struct AssetEntry {
    String path; // relative to the runtime asset root, no ".bin" suffix
    AssetKind kind     = AssetKind::UNKNOWN;
    AssetOrigin origin = AssetOrigin::File;
    AssetStatus status = AssetStatus::Ready;

    int64_t source_mtime = 0;   // raw source, clock ticks
    uint64_t source_size = 0;   // raw source, bytes

    int64_t compiled_mtime = 0; // compiled .bin, clock ticks
    uint64_t compiled_size = 0; // compiled .bin, bytes

    uint64_t source_hash = 0;   // content hash (incl. #include deps), or procedural params hash
    String source_path;         // absolute path of the raw source (file assets)
    String last_error;          // non-empty when status == Error
};

/**
 * @brief Produces (or regenerates) a procedural asset.
 *
 * The generator talks to ResourceLoader/RenderService and binds its result
 * under the asset's path (typically via ResourceLoader::add(res, path)). Runs
 * on the main thread.
 */
using AssetGenerateFn = std::function<RID( ResourceLoader& loader, RenderService& renderer )>;

StringView asset_status_name( AssetStatus status );
StringView asset_origin_name( AssetOrigin origin );

/**
 * @brief One manifest record: what we know about an asset's source at import time.
 */
struct ManifestRecord {
    int64_t source_mtime = 0;
    uint64_t source_size = 0;
    uint64_t source_hash = 0;

    bool operator==( const ManifestRecord& ) const = default;
};

/**
 * @brief The persisted import manifest (versioned, tab-separated, atomic write).
 *
 * Records only hold "known-good" file assets: the source metadata and content
 * hash an asset had when it last matched its compiled artifact. Stale/failed
 * assets are deliberately absent so they are re-blessed on their next import.
 * Procedural assets are never persisted.
 */
struct ImportManifest {
    HashMap<String, ManifestRecord> records;
    String ctx;                // header token: hash of asset root + source dirs
    bool valid_header = false; // true when the file carried a recognized "v1" header

    void clear();
    void load( StringView path );
    bool save( StringView path ) const;
};

/** @brief A regenerable procedural asset registered by game/engine code. */
struct ProceduralRecord {
    AssetKind kind = AssetKind::UNKNOWN;
    AssetGenerateFn generate;
};

/**
 * @brief Pure editor-side database of file-backed and procedural assets.
 *
 * Owns the authoritative list the panels query, plus the import manifest and
 * procedural registrations. It performs no compilation or hashing: `scan` walks
 * the runtime asset root (deriving staleness from the manifest), and the
 * AssetImporter recomputes hashes on its worker. GPU thumbnails live in a
 * separate ThumbnailCache.
 */
class AssetDatabase {
public:
    /** @brief Set the source dirs / search paths / asset root / manifest path. */
    void configure( AssetPaths p_paths );

    /**
     * @brief Rebuild file entries and reconcile procedural entries.
     *
     * Never hashes on the caller's thread: entries whose source hash cannot be
     * reused from the manifest are marked Compiling and queued on the importer,
     * which resolves them (and the manifest) on its worker via mark_scanned().
     */
    void scan( ResourceLoader& loader, AssetImporter& importer );

    bool get_scanned() const
    {
        return is_scanned;
    }

    Span<const AssetEntry> get_entries() const
    {
        return Span<const AssetEntry>( assets.data(), assets.size() );
    }

    AssetEntry* find_mut( StringView path ) [[clang::lifetimebound]];

    const AssetEntry* find( StringView path ) const [[clang::lifetimebound]];

    /**
     * @brief Register a regenerable procedural asset. Rejected if the path is
     * already a file asset.
     */
    bool register_procedural( StringView path, AssetKind kind, AssetGenerateFn generator );

    /** @brief Remove a procedural registration (and its entry, if any). */
    void unregister_procedural( StringView path );

    /** @brief Mark a procedural (or file) asset as needing rework. */
    void mark_dirty( StringView path );

    /** @brief Whether this procedural path has a registered generator (regenerable). */
    bool has_generator( StringView path ) const;

    /** @brief Record a reimport/generation job as in flight. */
    void mark_compiling( StringView path );

    /**
     * @brief Commit a successful import: update the entry and manifest from the
     * source hash/metadata computed on the import worker.
     */
    void mark_imported( StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size );

    /**
     * @brief Resolve a scan-time hash (computed on the import worker) into a
     * Ready/Dirty status and the manifest record.
     */
    void mark_scanned( StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size );

    /** @brief Record a failed import so the row can show it. */
    void mark_failed( StringView path, StringView error );

    /** @brief Persist the manifest if pending mark_imported/mark_scanned updates. */
    void flush_manifest();

    /**
     * @brief Load a file asset, or run a procedural generator when dirty/forced.
     */
    RID load_or_generate( StringView path, ResourceLoader& loader, RenderService& renderer, bool force = false );

private:
    AssetEntry* ensure_procedural_entry( StringView path ) [[clang::lifetimebound]];
    void rebuild_index();

    DynamicArray<AssetEntry> assets;   // sorted by path; stable iteration order
    HashMap<String, uint32_t> by_path; // path -> index into assets
    AssetPaths paths;
    ImportManifest manifest;
    HashMap<String, ProceduralRecord> generators;
    bool manifest_dirty = false;
    bool is_scanned = false;
};

} // namespace nc::editor
