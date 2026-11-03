#pragma once

#include <cstdint>

#include <ncore/core/collection.h>
#include <ncore/structs/rid.h>

#include <io/asset_paths.h>

namespace nc {
class ResourceLoader;
} // namespace nc

namespace nc::editor {

class AssetImporter;

/**
 * @brief Current state of an asset, computed during scan and updated by the
 * import service.
 */
enum class AssetStatus : uint8_t {
    READY = 0,      // compiled matches source
    DIRTY,          // source/params newer than the last compile
    MISSING_SOURCE, // no raw source found
    COMPILING,      // reimport job in flight
    ERROR,          // last compile failed (see last_error)
};

struct AssetEntry {
    String path; // relative to the runtime asset root, no ".bin" suffix
    AssetKind kind     = AssetKind::UNKNOWN;
    AssetStatus status = AssetStatus::READY;

    int64_t source_mtime = 0;   // raw source, clock ticks
    uint64_t source_size = 0;   // raw source, bytes

    int64_t compiled_mtime = 0; // compiled .bin, clock ticks
    uint64_t compiled_size = 0; // compiled .bin, bytes

    uint64_t source_hash = 0;   // content hash (incl. #include deps)
    String source_path;         // absolute path of the raw source
    String last_error;          // non-empty when status == ERROR
};

StringView asset_status_name( AssetStatus status );

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
 * Records only hold "known-good" assets: the source metadata and content hash an
 * asset had when it last matched its compiled artifact. Stale/failed assets are
 * deliberately absent so they are re-blessed on their next import.
 */
struct ImportManifest {
    HashMap<String, ManifestRecord> records;
    String ctx;                // header token: hash of asset root + source dirs
    bool valid_header = false; // true when the file carried a recognized "v1" header

    void clear();
    void load( StringView path );
    bool save( StringView path ) const;
};

/**
 * @brief Pure editor-side database of file-backed assets.
 *
 * Owns the authoritative list the panels query, plus the import manifest. It
 * performs no compilation or hashing: `scan` walks the runtime asset root
 * (deriving staleness from the manifest), and the AssetImporter recomputes
 * hashes on its worker. GPU thumbnails live in a separate ThumbnailCache.
 */
class AssetDatabase {
public:
    /** @brief Set the source dirs / search paths / asset root / manifest path. */
    void configure( AssetPaths p_paths );

    /**
     * @brief Rebuild the file entries from the asset root.
     *
     * Never hashes on the caller's thread: entries whose source hash cannot be
     * reused from the manifest are marked COMPILING and queued on the importer,
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

    AssetEntry* find_mut( StringView path ) NC_LIFETIME_BOUND;

    const AssetEntry* find( StringView path ) const NC_LIFETIME_BOUND;

    /** @brief Mark an asset as needing rework. */
    void mark_dirty( StringView path );

    /** @brief Record a reimport/generation job as in flight. */
    void mark_compiling( StringView path );

    /**
     * @brief Commit a successful import: update the entry and manifest from the
     * source hash/metadata computed on the import worker.
     */
    void mark_imported( StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size );

    /**
     * @brief Resolve a scan-time hash (computed on the import worker) into a
     * READY/DIRTY status and the manifest record.
     */
    void mark_scanned( StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size );

    /** @brief Record a failed import so the row can show it. */
    void mark_failed( StringView path, StringView error );

    /** @brief Persist the manifest if pending mark_imported/mark_scanned updates. */
    void flush_manifest();

    /** @brief Load a file asset (cached unless `force`). */
    RID ensure( StringView path, ResourceLoader& loader, bool force = false );

private:
    void rebuild_index();

    DynamicArray<AssetEntry> assets;   // sorted by path; stable iteration order
    HashMap<String, uint32_t> by_path; // path -> index into assets
    AssetPaths paths;
    ImportManifest manifest;
    bool manifest_dirty = false;
    bool is_scanned     = false;
};

} // namespace nc::editor
