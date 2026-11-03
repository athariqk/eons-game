#include "editor_assets.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <ncore/runtime/io/resource_loader.h>
#include <ncore/utils/log.h>

#include "asset_importer.h"

namespace nc::editor {

namespace {

// Stable context token: hashing the asset root + source dirs means a project or
// source-dir change invalidates the manifest and forces one full re-hash.
String manifest_ctx( const AssetPaths& paths )
{
    uint64_t h = fnv1a( paths.asset_root );
    for (const auto& dir : paths.source_dirs)
        h = fnv1a( dir, h );

    char buffer[17];
    std::snprintf( buffer, sizeof( buffer ), "%016llx", static_cast<unsigned long long>( h ) );
    return String( buffer );
}

bool records_equal( const HashMap<String, ManifestRecord>& a, const HashMap<String, ManifestRecord>& b )
{
    if (a.size() != b.size())
        return false;
    for (const auto& [path, record] : a) {
        const auto it = b.find( path );
        if (it == b.end() || !( it->second == record ))
            return false;
    }
    return true;
}

} // namespace

StringView asset_status_name( AssetStatus status )
{
    switch (status) {
        case AssetStatus::READY:
            return "ready";
        case AssetStatus::DIRTY:
            return "dirty";
        case AssetStatus::MISSING_SOURCE:
            return "no source";
        case AssetStatus::COMPILING:
            return "compiling";
        case AssetStatus::ERROR:
            return "error";
    }
    return "";
}

//------------------------------------------------------------------------------
// ImportManifest
//------------------------------------------------------------------------------

void ImportManifest::clear()
{
    records.clear();
    ctx.clear();
    valid_header = false;
}

void ImportManifest::load( StringView path )
{
    clear();

    std::ifstream in( std::string( path.data(), path.size() ), std::ios::binary );
    if (!in.is_open())
        return;

    std::string line;
    if (!std::getline( in, line ) || line != "v1")
        return; // missing/unknown version => caller treats it as empty

    valid_header = true;

    while (std::getline( in, line )) {
        if (line.empty())
            continue;

        if (line[0] == '#') {
            if (line.rfind( "# ctx=", 0 ) == 0)
                ctx = String( line.c_str() + 6 );
            continue;
        }

        const auto t1 = line.find( '\t' );
        if (t1 == std::string::npos)
            continue;
        const auto t2 = line.find( '\t', t1 + 1 );
        if (t2 == std::string::npos)
            continue;
        const auto t3 = line.find( '\t', t2 + 1 );
        if (t3 == std::string::npos)
            continue;

        ManifestRecord record;
        record.source_mtime = std::strtoll( line.c_str() + t1 + 1, nullptr, 10 );
        record.source_size  = std::strtoull( line.c_str() + t2 + 1, nullptr, 10 );
        record.source_hash  = std::strtoull( line.c_str() + t3 + 1, nullptr, 16 );

        records[String( line.c_str(), t1 )] = record;
    }
}

bool ImportManifest::save( StringView path ) const
{
    const std::string path_str( path.data(), path.size() );
    const std::filesystem::path manifest_path( path_str );
    const std::string tmp = path_str + ".tmp";

    std::error_code ec;
    if (manifest_path.has_parent_path())
        std::filesystem::create_directories( manifest_path.parent_path(), ec );

    // Deterministic order so the file stays diffable across identical runs.
    DynamicArray<String> keys;
    keys.reserve( records.size() );
    for (const auto& entry : records)
        keys.push_back( entry.first );
    std::sort( keys.begin(), keys.end() );

    {
        std::ofstream out( tmp, std::ios::trunc );
        if (!out.is_open())
            return false;

        out << "v1\n";
        out << "# ctx=" << ctx.c_str() << '\n';
        for (const auto& key : keys) {
            const ManifestRecord& record = records.at( key );
            char hash[17];
            std::snprintf( hash, sizeof( hash ), "%016llx", static_cast<unsigned long long>( record.source_hash ) );
            out << key.c_str() << '\t' << record.source_mtime << '\t' << record.source_size << '\t' << hash << '\n';
        }
    }

    std::filesystem::remove( path_str, ec );
    ec.clear();
    std::filesystem::rename( tmp, path_str, ec );
    return !ec;
}

//------------------------------------------------------------------------------
// AssetDatabase
//------------------------------------------------------------------------------

void AssetDatabase::configure( AssetPaths p_paths )
{
    paths = std::move( p_paths );
}

void AssetDatabase::rebuild_index()
{
    std::sort( assets.begin(), assets.end(), []( const AssetEntry& a, const AssetEntry& b ) {
        return a.path < b.path;
    } );
    by_path.clear();
    for (size_t i = 0; i < assets.size(); ++i)
        by_path.emplace( assets[i].path, static_cast<uint32_t>( i ) );
}

void AssetDatabase::scan( ResourceLoader& loader, AssetImporter& importer )
{
    assets.clear();
    by_path.clear();
    is_scanned = true;

    const std::filesystem::path root( paths.asset_root.c_str() );

    manifest.load( paths.manifest_path );
    const String ctx           = manifest_ctx( paths );
    const bool manifest_usable = manifest.valid_header && manifest.ctx == ctx;

    HashMap<String, ManifestRecord> next;
    HashSet<String> present;
    DynamicArray<String> need_hash;

    std::error_code ec;
    if (std::filesystem::exists( root, ec )) {
        for (auto it = std::filesystem::recursive_directory_iterator( root, ec );
             it != std::filesystem::recursive_directory_iterator(); it.increment( ec )) {
            if (ec)
                break;
            if (!it->is_regular_file( ec ) || it->path().extension() != ".bin")
                continue;

            auto rel = std::filesystem::relative( it->path(), root, ec );
            if (ec)
                continue;

            std::string rel_str = rel.string();
            rel_str.resize( rel_str.size() - 4 ); // strip ".bin"
            std::replace( rel_str.begin(), rel_str.end(), '\\', '/' );

            AssetEntry entry;
            entry.path = String( rel_str.c_str(), rel_str.size() );

            uint32_t format_id = 0;
            entry.kind = read_format_id( it->path(), format_id ) ? kind_from_format( format_id ) : AssetKind::UNKNOWN;
            entry.compiled_size  = it->file_size( ec );
            entry.compiled_mtime = it->last_write_time( ec ).time_since_epoch().count();

            if (!find_source( entry.path, paths, entry.source_path )) {
                entry.status = AssetStatus::MISSING_SOURCE;
            } else {
                ec.clear();
                entry.source_mtime =
                    std::filesystem::last_write_time( entry.source_path.c_str(), ec ).time_since_epoch().count();
                ec.clear();
                entry.source_size = std::filesystem::file_size( entry.source_path.c_str(), ec );

                const ManifestRecord* known = nullptr;
                if (manifest_usable) {
                    const auto known_it = manifest.records.find( entry.path );
                    if (known_it != manifest.records.end())
                        known = &known_it->second;
                }

                const bool reuse_hash = known && !kind_has_includes( entry.kind ) &&
                                        known->source_mtime == entry.source_mtime &&
                                        known->source_size == entry.source_size;

                if (reuse_hash) {
                    entry.source_hash = known->source_hash;
                    entry.status      = AssetStatus::READY;
                    next.emplace( entry.path, *known );
                } else {
                    // Needs a (re)hash, which runs on the import worker. Keep the
                    // previous record so mark_scanned can compare against it.
                    entry.source_hash = known ? known->source_hash : 0;
                    entry.status      = AssetStatus::COMPILING;
                    if (known)
                        next.emplace( entry.path, *known );
                    need_hash.push_back( entry.path );
                }
            }

            present.insert( entry.path );
            assets.push_back( std::move( entry ) );
        }
    }

    rebuild_index();

    if (!manifest.valid_header || manifest.ctx != ctx || !records_equal( manifest.records, next )) {
        manifest.records      = std::move( next );
        manifest.ctx          = ctx;
        manifest.valid_header = true;
        manifest.save( paths.manifest_path );
    }

    // Queue the expensive hashes on the worker so the main thread stays idle.
    for (const auto& path : need_hash)
        importer.enqueue_hash( path );

    NC_LOG_INFO_C( log::IO, "AssetDatabase: {} assets under '{}'", assets.size(), paths.asset_root.c_str() );
}

AssetEntry* AssetDatabase::find_mut( StringView path )
{
    const auto it = by_path.find( String( path.data(), path.size() ) );
    if (it == by_path.end())
        return nullptr;
    return &assets[it->second];
}

const AssetEntry* AssetDatabase::find( StringView path ) const
{
    const auto it = by_path.find( String( path.data(), path.size() ) );
    if (it == by_path.end())
        return nullptr;
    return &assets[it->second];
}

void AssetDatabase::mark_dirty( StringView path )
{
    if (auto* entry = find_mut( path ))
        entry->status = AssetStatus::DIRTY;
}

void AssetDatabase::mark_compiling( StringView path )
{
    if (auto* entry = find_mut( path ))
        entry->status = AssetStatus::COMPILING;
}

void AssetDatabase::mark_imported( StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size )
{
    auto* entry = find_mut( path );
    if (!entry)
        return;

    find_source( entry->path, paths, entry->source_path ); // refresh source path (for display)

    std::error_code ec;
    entry->source_mtime = source_mtime;
    entry->source_size  = source_size;
    entry->source_hash  = source_hash;

    const auto compiled =
        std::filesystem::path( paths.asset_root.c_str() ) / ( std::string( entry->path.c_str() ) + ".bin" );
    entry->compiled_mtime = std::filesystem::last_write_time( compiled, ec ).time_since_epoch().count();
    ec.clear();
    entry->compiled_size = std::filesystem::file_size( compiled, ec );
    entry->status        = AssetStatus::READY;
    entry->last_error.clear();

    if (!manifest.valid_header)
        manifest.load( paths.manifest_path );

    manifest.ctx                  = manifest_ctx( paths );
    manifest.valid_header         = true;
    manifest.records[entry->path] = ManifestRecord{ source_mtime, source_size, source_hash };
    manifest_dirty                = true;
}

void AssetDatabase::mark_scanned( StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size )
{
    auto* entry = find_mut( path );
    if (!entry)
        return;

    entry->source_hash  = source_hash;
    entry->source_mtime = source_mtime;
    entry->source_size  = source_size;

    if (!manifest.valid_header)
        manifest.load( paths.manifest_path );

    const auto known_it = manifest.records.find( entry->path );
    const bool known    = known_it != manifest.records.end();
    const bool matches  = known && known_it->second.source_hash == source_hash;

    if (known && matches) {
        entry->status = AssetStatus::READY;
    } else if (known) {
        entry->status = AssetStatus::DIRTY;
        manifest.records.erase( known_it ); // stale: re-blessed on its next import
        manifest_dirty = true;
    } else {
        // New to the manifest: bless it once via the mtime fallback.
        entry->status = source_mtime > entry->compiled_mtime ? AssetStatus::DIRTY : AssetStatus::READY;
        if (entry->status == AssetStatus::READY) {
            manifest.records[entry->path] = ManifestRecord{ source_mtime, source_size, source_hash };
            manifest_dirty                = true;
        }
    }

    manifest.ctx          = manifest_ctx( paths );
    manifest.valid_header = true;
}

void AssetDatabase::flush_manifest()
{
    if (!manifest_dirty)
        return;
    manifest_dirty = false;
    manifest.save( paths.manifest_path );
}

void AssetDatabase::mark_failed( StringView path, StringView error )
{
    if (auto* entry = find_mut( path )) {
        entry->status     = AssetStatus::ERROR;
        entry->last_error = String( error.data(), error.size() );
    }
}

RID AssetDatabase::ensure( StringView path, ResourceLoader& loader, bool force )
{
    return loader.load( String( path.data(), path.size() ), force );
}

} // namespace nc::editor
