// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: editor_assets.h

#pragma once

#include <cstdint>

#include <ncore/core/collection.h>
#include <ncore/object/reference.h>
#include <ncore/structs/rid.h>

#include <editor/io/asset_importer.h>
#include <editor/io/asset_paths.h>
#include <editor/io/thumbnail_cache.h>

namespace nc {
class AssetService;
class RefCounted;
class ResourceLoader;
class Scene;
} // namespace nc

namespace sea::editor {

/**
 * @brief Current state of an asset, computed during scan and updated by the
 * import driver.
 */
enum class AssetStatus : uint8_t {
    READY = 0,      // compiled matches source
    DIRTY,          // source/params newer than the last compile
    MISSING_SOURCE, // no raw source found
    COMPILING,      // reimport job in flight
    ERROR,          // last compile failed (see last_error)
};

struct AssetEntry {
    nc::String path; // relative to the runtime asset root, no ".bin" suffix
    AssetKind kind     = AssetKind::UNKNOWN;
    AssetStatus status = AssetStatus::READY;

    int64_t source_mtime = 0;   // raw source, clock ticks
    uint64_t source_size = 0;   // raw source, bytes

    int64_t compiled_mtime = 0; // compiled .bin, clock ticks
    uint64_t compiled_size = 0; // compiled .bin, bytes

    uint64_t source_hash = 0;   // content hash (incl. #include deps)
    nc::String source_path;     // absolute path of the raw source
    nc::String last_error;      // non-empty when status == ERROR
};

nc::StringView asset_status_name( AssetStatus status );

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
    nc::HashMap<nc::String, ManifestRecord> records;
    nc::String ctx;            // header token: hash of asset root + source dirs
    bool valid_header = false; // true when the file carried a recognized "v1" header

    void clear();
    void load( nc::StringView path );
    bool save( nc::StringView path ) const;
};

/**
 * @brief The editor's asset subsystem.
 */
class EditorAssets final : private IAssetImportSink {
public:
    EditorAssets( nc::Scene& p_scene NC_LIFETIME_BOUND, nc::AssetService* p_compiler );
    ~EditorAssets() override;

    EditorAssets( const EditorAssets& )            = delete;
    EditorAssets& operator=( const EditorAssets& ) = delete;

    /**
     * @brief Set the source dirs / search paths / asset root / manifest path.
     */
    void configure( AssetPaths p_paths );

    /**
     * @brief Scan-if-needed, pump finished imports, reset the thumbnail budget.
     */
    void begin_frame();

    /**
     * @brief Stop the worker and drop cached thumbnail handles. Safe twice.
     */
    void shutdown();

    nc::Span<const AssetEntry> get_entries() const
    {
        return nc::Span<const AssetEntry>( assets.data(), assets.size() );
    }

    AssetEntry* find_mut( nc::StringView path ) NC_LIFETIME_BOUND;

    const AssetEntry* find( nc::StringView path ) const NC_LIFETIME_BOUND;

    void rescan();
    void reimport( nc::StringView p_path );
    void reload( nc::StringView p_path );

    /**
     * @brief Load a file asset (cached unless `force`).
     */
    nc::RID ensure( nc::StringView p_path, bool force = false );

    /**
     * @brief Load an asset and return its live instance, or null.
     *
     * Main thread only. The loader keeps the resource alive while it is cached,
     * so the returned pointer is valid until an unload.
     */
    nc::RefCounted* resolve( nc::StringView p_path );

    /**
     * @brief Load an asset and return a typed strong handle (or null).
     *
     * The only entry point panels use to obtain a resource; they never touch
     * the scene's ResourceLoader directly.
     */
    template<typename T>
    nc::Ref<T> load( nc::StringView p_path )
    {
        nc::RefCounted* rc = resolve( p_path );
        return rc ? nc::Ref<T>::share( static_cast<T*>( rc ) ) : nc::Ref<T>();
    }

    /** @brief Prepared thumbnail handle for an image asset. */
    nc::RID thumbnail( nc::StringView p_path, bool reload = false );

    bool last_result( nc::StringView p_path, AssetImportResult& out );

    bool can_import() const;
    bool busy();
    int pending();

    bool get_scanned() const
    {
        return is_scanned;
    }

    const AssetPaths& paths() const NC_LIFETIME_BOUND
    {
        return asset_paths;
    }

    /**
     * @brief Check source files on disk for external modifications (mtime).
     * If a source is newer than our last record, mark dirty and enqueue reimport.
     * Call this every frame (throttled inside). Makes hot reload "just work".
     */
    void poll_source_changes();

    /**
     * @brief After a shader hot reload, walk live materials in the scene and re-point
     * their shader Ref to the freshly loaded one (so revision checks and bytecode update).
     * This enables reliable live editing of shaders while the simulation continues.
     */
    void refresh_materials_using_shader( nc::StringView shader_asset_path );

private:
    /** @brief Mark an asset as needing rework. */
    void mark_dirty( nc::StringView path );

    /** @brief Record a reimport/generation job as in flight. */
    void mark_compiling( nc::StringView path );

    /**
     * @brief Commit a successful import: update the entry and manifest from the
     * source hash/metadata computed on the import worker.
     */
    void mark_imported( nc::StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size );

    /**
     * @brief Resolve a scan-time hash (computed on the import worker) into a
     * READY/DIRTY status and the manifest record.
     */
    void mark_scanned( nc::StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size );

    /** @brief Record a failed import so the row can show it. */
    void mark_failed( nc::StringView path, nc::StringView error );

    /** @brief Persist the manifest if pending mark_imported/mark_scanned updates. */
    void flush_manifest();

private:
    void rebuild_index();

    void on_import_started( nc::StringView p_path ) override;
    void on_import_finished( const AssetImportResult& p_result ) override;

    nc::Scene& scene;
    AssetPaths asset_paths;
    AssetImporter importer;
    ThumbnailCache thumbnails;

    nc::DynamicArray<AssetEntry> assets;       // sorted by path; stable iteration order
    nc::HashMap<nc::String, uint32_t> by_path; // path -> index into assets
    ImportManifest manifest;
    bool manifest_dirty = false;
    bool is_scanned     = false;
};

} // namespace sea::editor
