// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: asset_importer.h

#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

#include <ncore/core/collection.h>

#include <editor/io/asset_paths.h>

namespace nc {
class AssetService;
} // namespace nc

namespace sea::editor {

struct AssetImportResult {
    nc::String path;
    bool ok = false;
    nc::String message;
    double ms = 0.0;

    uint64_t source_hash = 0; // computed on the worker (file assets)
    int64_t source_mtime = 0; // raw source, clock ticks
    uint64_t source_size = 0; // raw source, bytes

    bool hash_only = false;   // scan-time reconcile: hash only, no compile/reload
};

/**
 * @brief Main-thread receiver of finished import jobs.
 *
 * The driver deliberately knows nothing about ResourceLoader or AssetDatabase:
 * the editor facade implements this sink, reloads on the main thread and
 * commits the result into the catalog.
 */
class IAssetImportSink {
public:
    virtual ~IAssetImportSink() = default;

    /** @brief A job was accepted for @p path (called on the main thread). */
    virtual void on_import_started( nc::StringView p_path ) = 0;

    /** @brief A job finished; apply it on the main thread. */
    virtual void on_import_finished( const AssetImportResult& p_result ) = 0;
};

/**
 * @brief Recompiles raw assets on a background thread.
 *
 * The UI only enqueues. A single worker thread runs nc::AssetService::compile and
 * (for file assets) recomputes the source hash; the main thread (via `pump`)
 * then hands results to the sink, which reloads through ResourceLoader and
 * commits them into the catalog. This keeps every ResourceLoader / RenderService
 * / index access on the main thread, so none of them need to be thread-safe.
 */
class AssetImporter {
public:
    AssetImporter();
    ~AssetImporter();

    AssetImporter( const AssetImporter& )            = delete;
    AssetImporter& operator=( const AssetImporter& ) = delete;

    void init( nc::AssetService& p_assets, AssetPaths p_paths, IAssetImportSink& p_sink );

    /** @brief Stop the worker and drop queued work. Safe to call twice. */
    void shutdown();

    bool ready() const
    {
        return assets != nullptr;
    }

    /** @brief Queue one asset for reimport (no-op when already queued). */
    void enqueue( nc::StringView p_asset_path );

    /** @brief Queue one asset for a scan-time hash (no compile/reload). */
    void enqueue_hash( nc::StringView p_asset_path );

    /** @brief Apply finished jobs on the main thread. Call once per frame. */
    void pump();

    int get_pending();

    bool busy();

    /** @brief Copy the last result for a path. Returns false when none yet. */
    bool last_result( nc::StringView p_asset_path, AssetImportResult& out );

private:
    struct PendingJob {
        nc::String path;
        bool hash_only = false;
    };

    void enqueue_job( nc::StringView p_asset_path, bool hash_only );
    void compute_source_info( const nc::String& source, AssetImportResult& result ); // worker thread only
    AssetImportResult compile_file( nc::StringView p_asset_path );                   // worker thread only
    AssetImportResult hash_source( nc::StringView p_asset_path );                    // worker thread only

    nc::AssetService* assets = nullptr;
    IAssetImportSink* sink   = nullptr;
    AssetPaths paths;

    std::mutex mutex;
    std::condition_variable cv;
    nc::DynamicArray<PendingJob> pending_queue;         // guarded by mutex
    nc::DynamicArray<AssetImportResult> ready_queue;    // guarded by mutex
    nc::HashMap<nc::String, AssetImportResult> results; // guarded by mutex
    bool shutting_down = false;                         // guarded by mutex
    std::jthread worker;                                // declared last: destroyed first, so it joins before the above
};

} // namespace sea::editor
