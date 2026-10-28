#include "asset_importer.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>

#include <ncore/application.h>
#include <ncore/core/errors.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/utils/log.h>

#include "editor_assets.h"

namespace nc::editor {

AssetImporter::AssetImporter() = default;

AssetImporter::~AssetImporter()
{
    {
        std::lock_guard<std::mutex> lock( mutex );
        shutting_down = true;
    }
    cv.notify_all();
    // `worker` is declared last, so its destructor joins before mutex/cv/queues are destroyed.
}

void AssetImporter::init(
    const AppContext& p_app, ResourceLoader& p_loader, AssetService& p_assets, AssetDatabase& p_db
)
{
    assets = &p_assets;
    loader = &p_loader;
    db     = &p_db;

    paths = discover_asset_paths( p_app, p_loader );
    db->configure( paths );

    worker = std::jthread( [this]() {
        for (;;) {
            PendingJob job;
            {
                std::unique_lock<std::mutex> lock( mutex );
                cv.wait( lock, [this] { return shutting_down || !pending_queue.empty(); } );
                if (shutting_down && pending_queue.empty())
                    return;
                job = std::move( pending_queue.front() );
                pending_queue.erase( pending_queue.begin() );
            }

            auto result = job.hash_only ? hash_source( job.path ) : compile_file( job.path );

            {
                std::lock_guard<std::mutex> lock( mutex );
                ready_queue.push_back( std::move( result ) );
                results[job.path] = ready_queue.back();
            }
        }
    } );

    NC_LOG_INFO_C(
        log::IO, "AssetImporter: {} source dirs, {} shader search paths", paths.source_dirs.size(),
        paths.search_paths.size()
    );
}

void AssetImporter::enqueue( StringView p_asset_path )
{
    enqueue_job( p_asset_path, /*hash_only=*/false );
}

void AssetImporter::enqueue_hash( StringView p_asset_path )
{
    enqueue_job( p_asset_path, /*hash_only=*/true );
}

void AssetImporter::enqueue_job( StringView p_asset_path, bool hash_only )
{
    if (p_asset_path.empty())
        return;

    const String key( p_asset_path.data(), p_asset_path.size() );

    {
        std::lock_guard<std::mutex> lock( mutex );
        for (auto& job : pending_queue) {
            if (job.path != key)
                continue;
            // A compile request supersedes a pending hash-only request.
            job.hash_only = job.hash_only && hash_only;
            return;
        }
        pending_queue.push_back( PendingJob{ key, hash_only } );
    }
    cv.notify_one();

    if (db)
        db->mark_compiling( key );
}

void AssetImporter::pump()
{
    DynamicArray<AssetImportResult> done;
    {
        std::lock_guard<std::mutex> lock( mutex );
        done.swap( ready_queue );
    }

    for (auto& result : done) {
        if (result.hash_only) {
            if (result.ok && db)
                db->mark_scanned( result.path, result.source_hash, result.source_mtime, result.source_size );
            else if (db)
                db->mark_failed( result.path, result.message );
        } else if (result.ok) {
            if (loader)
                loader->load( result.path, true );
            if (db)
                db->mark_imported( result.path, result.source_hash, result.source_mtime, result.source_size );
        } else if (db) {
            db->mark_failed( result.path, result.message );
        }
    }

    if (db)
        db->flush_manifest();
}

int AssetImporter::get_pending()
{
    std::lock_guard<std::mutex> lock( mutex );
    return static_cast<int>( pending_queue.size() + ready_queue.size() );
}

bool AssetImporter::busy()
{
    std::lock_guard<std::mutex> lock( mutex );
    return !pending_queue.empty() || !ready_queue.empty();
}

bool AssetImporter::last_result( StringView p_asset_path, AssetImportResult& out )
{
    std::lock_guard<std::mutex> lock( mutex );
    const auto it = results.find( String( p_asset_path.data(), p_asset_path.size() ) );
    if (it == results.end())
        return false;
    out = it->second;
    return true;
}

AssetImportResult AssetImporter::compile_file( StringView p_asset_path )
{
    const auto start = std::chrono::steady_clock::now();
    const String key( p_asset_path.data(), p_asset_path.size() );

    auto finish = [&]( bool p_ok, String p_message ) {
        AssetImportResult result;
        result.path    = key;
        result.ok      = p_ok;
        result.message = std::move( p_message );
        result.ms = std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - start ).count();
        return result;
    };

    String source;
    if (!find_source( key, paths, source ))
        return finish( false, "raw source not found in the configured source dirs" );

    const std::filesystem::path compiled =
        std::filesystem::path( paths.asset_root.c_str() ) / ( std::string( key.c_str() ) + ".bin" );
    const std::string temp = compiled.string() + ".tmp";

    std::error_code ec;
    std::filesystem::remove( temp, ec );

    if (assets->compile( source, String( temp.c_str() ), paths.search_paths, true ) != Error::OK) {
        std::filesystem::remove( temp, ec );
        return finish( false, "compile failed (see Logs)" );
    }

    std::filesystem::remove( compiled, ec );
    std::filesystem::rename( temp, compiled, ec );
    if (ec) {
        std::filesystem::remove( temp, ec );
        return finish( false, "failed to replace " + String( compiled.string().c_str() ) );
    }

    // Hash the source on the worker (the expensive part). Shaders/materials
    // hash their #include tree; every other kind hashes its bytes.
    auto result = finish( true, "" );
    compute_source_info( source, result );
    NC_LOG_INFO_C( log::IO, "Compiled '{}' in {:.1f} ms", key, result.ms );
    return result;
}

void AssetImporter::compute_source_info( const String& source, AssetImportResult& result )
{
    const bool has_includes = source.ends_with( ".slang" );
    result.source_hash = has_includes ? hash_source_tree( source.c_str(), paths ) : hash_file( source.c_str() );

    std::error_code ec;
    result.source_mtime = std::filesystem::last_write_time( source.c_str(), ec ).time_since_epoch().count();
    ec.clear();
    result.source_size = std::filesystem::file_size( source.c_str(), ec );
}

AssetImportResult AssetImporter::hash_source( StringView p_asset_path )
{
    const auto start = std::chrono::steady_clock::now();
    const String key( p_asset_path.data(), p_asset_path.size() );

    auto finish = [&]( bool p_ok, String p_message ) {
        AssetImportResult result;
        result.path      = key;
        result.ok        = p_ok;
        result.message   = std::move( p_message );
        result.ms        = std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - start ).count();
        result.hash_only = true;
        return result;
    };

    String source;
    if (!find_source( key, paths, source ))
        return finish( false, "raw source not found in the configured source dirs" );

    auto result = finish( true, "" );
    compute_source_info( source, result );
    return result;
}

} // namespace nc::editor
