#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

#include <ncore/core/collection.h>

#include <io/asset_paths.h>

namespace nc {
class AssetService;
class ResourceLoader;
struct AppContext;
} // namespace nc

namespace nc::editor {

class AssetDatabase;

struct AssetImportResult {
    String path;
    bool ok = false;
    String message;
    double ms = 0.0;

    uint64_t source_hash  = 0; // computed on the worker (file assets)
    int64_t source_mtime  = 0; // raw source, clock ticks
    uint64_t source_size  = 0; // raw source, bytes

    bool hash_only = false;    // scan-time reconcile: hash only, no compile/reload
};

/**
 * @brief Recompiles raw assets on a background thread and reloads the results.
 *
 * The UI only enqueues. A single worker thread runs AssetService::compile and
 * (for file assets) recomputes the source hash; the main thread (via `pump`)
 * then reloads through ResourceLoader and commits the result into AssetDatabase.
 * This keeps every ResourceLoader / RenderService / index access on the main
 * thread, so none of them need to be made thread-safe.
 */
class AssetImporter {
public:
    AssetImporter();
    ~AssetImporter();

    AssetImporter( const AssetImporter& )            = delete;
    AssetImporter& operator=( const AssetImporter& ) = delete;

    void init( const AppContext& p_app, ResourceLoader& p_loader, AssetService& p_assets, AssetDatabase& p_db );

    bool ready() const
    {
        return assets != nullptr;
    }

    /** @brief Queue one asset for reimport (no-op when already queued). */
    void enqueue( StringView p_asset_path );

    /** @brief Queue one asset for a scan-time hash (no compile/reload). */
    void enqueue_hash( StringView p_asset_path );

    /** @brief Apply finished jobs on the main thread. Call once per frame. */
    void pump();

    int get_pending();

    bool busy();

    /** @brief Copy the last result for a path. Returns false when none yet. */
    bool last_result( StringView p_asset_path, AssetImportResult& out );

private:
    struct PendingJob {
        String path;
        bool hash_only = false;
    };

    void enqueue_job( StringView p_asset_path, bool hash_only );
    void compute_source_info( const String& source, AssetImportResult& result ); // worker thread only
    AssetImportResult compile_file( StringView p_asset_path );                   // worker thread only
    AssetImportResult hash_source( StringView p_asset_path );                    // worker thread only

    AssetService* assets   = nullptr;
    ResourceLoader* loader = nullptr;
    AssetDatabase* db         = nullptr;

    AssetPaths paths;

    std::mutex mutex;
    std::condition_variable cv;
    DynamicArray<PendingJob> pending_queue;       // guarded by mutex
    DynamicArray<AssetImportResult> ready_queue;  // guarded by mutex
    HashMap<String, AssetImportResult> results;   // guarded by mutex
    bool shutting_down = false;                   // guarded by mutex
    std::jthread worker;                          // declared last: destroyed first, so it joins before the above
};

} // namespace nc::editor
