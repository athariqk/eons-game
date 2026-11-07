// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: editor_assets.cpp

#include "editor_assets.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

#include <ncore/app_context.h>
#include <ncore/runtime/components.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/utils/log.h>

#include "asset_importer.h"

namespace sea::editor {

namespace {

// Stable context token: hashing the asset root + source dirs means a project or
// source-dir change invalidates the manifest and forces one full re-hash.
nc::String manifest_ctx( const AssetPaths& paths )
{
    uint64_t h = fnv1a( paths.asset_root );
    for (const auto& dir : paths.source_dirs)
        h = fnv1a( dir, h );

    char buffer[17];
    std::snprintf( buffer, sizeof( buffer ), "%016llx", static_cast<unsigned long long>( h ) );
    return nc::String( buffer );
}

bool records_equal( const nc::HashMap<nc::String, ManifestRecord>& a, const nc::HashMap<nc::String, ManifestRecord>& b )
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

nc::StringView asset_status_name( AssetStatus status )
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

void ImportManifest::load( nc::StringView path )
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
                ctx = nc::String( line.c_str() + 6 );
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

        records[nc::String( line.c_str(), t1 )] = record;
    }
}

bool ImportManifest::save( nc::StringView path ) const
{
    const std::string path_str( path.data(), path.size() );
    const std::filesystem::path manifest_path( path_str );
    const std::string tmp = path_str + ".tmp";

    std::error_code ec;
    if (manifest_path.has_parent_path())
        std::filesystem::create_directories( manifest_path.parent_path(), ec );

    // Deterministic order so the file stays diffable across identical runs.
    nc::DynamicArray<nc::String> keys;
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
// EditorAssets
//------------------------------------------------------------------------------

EditorAssets::EditorAssets( nc::Scene& p_scene, nc::AssetService* p_compiler ) : scene( p_scene )
{
    if (auto* app = scene.get_app_ctx()) {
        asset_paths = discover_asset_paths( *app, scene.get_resource_loader() );
        configure( asset_paths );

        if (p_compiler)
            importer.init( *p_compiler, asset_paths, *this );
        else
            NC_LOG_WARN_C( nc::log::IO, "EditorAssets: no AssetService; import is disabled" );
    } else {
        NC_LOG_WARN_C( nc::log::IO, "EditorAssets: no AppContext; asset discovery skipped" );
    }
}

EditorAssets::~EditorAssets()
{
    shutdown();
}

void EditorAssets::configure( AssetPaths p_paths )
{
    asset_paths = std::move( p_paths );
}

void EditorAssets::begin_frame()
{
    thumbnails.begin_frame();
    importer.pump();
    flush_manifest();

    if (!get_scanned())
        rescan();

    // Robust + fun hot reload: detect external edits to sources without user clicking buttons.
    poll_source_changes();
}

void EditorAssets::shutdown()
{
    importer.shutdown();
    thumbnails.release_gpu();
}

AssetEntry* EditorAssets::find_mut( nc::StringView path )
{
    const auto it = by_path.find( nc::String( path.data(), path.size() ) );
    if (it == by_path.end())
        return nullptr;
    return &assets[it->second];
}

const AssetEntry* EditorAssets::find( nc::StringView path ) const
{
    const auto it = by_path.find( nc::String( path.data(), path.size() ) );
    if (it == by_path.end())
        return nullptr;
    return &assets[it->second];
}

void EditorAssets::rescan()
{
    auto p_importer = importer.ready() ? &importer : nullptr;

    assets.clear();
    by_path.clear();
    is_scanned = true;

    const std::filesystem::path root( asset_paths.asset_root.c_str() );

    manifest.load( asset_paths.manifest_path );
    const nc::String ctx       = manifest_ctx( asset_paths );
    const bool manifest_usable = manifest.valid_header && manifest.ctx == ctx;

    nc::HashMap<nc::String, ManifestRecord> next;
    nc::HashSet<nc::String> present;
    nc::DynamicArray<nc::String> need_hash;

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
            entry.path = nc::String( rel_str.c_str(), rel_str.size() );

            uint32_t format_id = 0;
            entry.kind = read_format_id( it->path(), format_id ) ? kind_from_format( format_id ) : AssetKind::UNKNOWN;
            entry.compiled_size  = it->file_size( ec );
            entry.compiled_mtime = it->last_write_time( ec ).time_since_epoch().count();

            if (!find_source( entry.path, asset_paths, entry.source_path )) {
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
                    if (known)
                        next.emplace( entry.path, *known );

                    if (p_importer && p_importer->ready()) {
                        entry.status = AssetStatus::COMPILING;
                        need_hash.push_back( entry.path );
                    } else {
                        // No compiler service: fall back to the mtime heuristic so
                        // the browser is still usable.
                        entry.status =
                            entry.source_mtime > entry.compiled_mtime ? AssetStatus::DIRTY : AssetStatus::READY;
                    }
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
        manifest.save( asset_paths.manifest_path );
    }

    // Queue the expensive hashes on the worker so the main thread stays idle.
    if (p_importer && p_importer->ready()) {
        for (const auto& path : need_hash)
            p_importer->enqueue_hash( path );
    }

    NC_LOG_INFO_C( nc::log::IO, "EditorAssets: {} assets under '{}'", assets.size(), asset_paths.asset_root.c_str() );
}

void EditorAssets::reimport( nc::StringView p_path )
{
    importer.enqueue( p_path );
}

void EditorAssets::reload( nc::StringView p_path )
{
    const AssetEntry* entry = find( p_path );

    if (entry && entry->kind == AssetKind::IMAGE) {
        thumbnails.texture_for( nc::String( p_path.data(), p_path.size() ), scene, true );
        return;
    }

    scene.get_resource_loader().load( nc::String( p_path.data(), p_path.size() ), true );

    if (entry && ( entry->kind == AssetKind::MATERIAL_SHADER || entry->kind == AssetKind::SHADER ||
                   nc::StringView( p_path ).ends_with( ".slang" ) )) {
        refresh_materials_using_shader( p_path );
    }
}

nc::RID EditorAssets::ensure( nc::StringView p_path, bool force )
{
    return scene.get_resource_loader().load( nc::String( p_path.data(), p_path.size() ), force );
}

nc::RefCounted* EditorAssets::resolve( nc::StringView p_path )
{
    const nc::RID rid = ensure( p_path );
    if (!rid.is_valid())
        return nullptr;

    nc::Ref<nc::IResource> resource = scene.get_resource_loader().get( rid );
    return resource.get();
}

nc::RID EditorAssets::thumbnail( nc::StringView p_path, bool reload )
{
    return thumbnails.texture_for( nc::String( p_path.data(), p_path.size() ), scene, reload );
}

bool EditorAssets::last_result( nc::StringView p_path, AssetImportResult& out )
{
    return importer.last_result( p_path, out );
}

bool EditorAssets::can_import() const
{
    return importer.ready();
}

bool EditorAssets::busy()
{
    return importer.busy();
}

int EditorAssets::pending()
{
    return importer.get_pending();
}

void EditorAssets::mark_dirty( nc::StringView path )
{
    if (auto* entry = find_mut( path ))
        entry->status = AssetStatus::DIRTY;
}

void EditorAssets::mark_compiling( nc::StringView path )
{
    if (auto* entry = find_mut( path ))
        entry->status = AssetStatus::COMPILING;
}

void EditorAssets::mark_imported(
    nc::StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size
)
{
    auto* entry = find_mut( path );
    if (!entry)
        return;

    find_source( entry->path, asset_paths, entry->source_path ); // refresh source path (for display)

    std::error_code ec;
    entry->source_mtime = source_mtime;
    entry->source_size  = source_size;
    entry->source_hash  = source_hash;

    const auto compiled =
        std::filesystem::path( asset_paths.asset_root.c_str() ) / ( std::string( entry->path.c_str() ) + ".bin" );
    entry->compiled_mtime = std::filesystem::last_write_time( compiled, ec ).time_since_epoch().count();
    ec.clear();
    entry->compiled_size = std::filesystem::file_size( compiled, ec );
    entry->status        = AssetStatus::READY;
    entry->last_error.clear();

    if (!manifest.valid_header)
        manifest.load( asset_paths.manifest_path );

    manifest.ctx                  = manifest_ctx( asset_paths );
    manifest.valid_header         = true;
    manifest.records[entry->path] = ManifestRecord{ source_mtime, source_size, source_hash };
    manifest_dirty                = true;
}

void EditorAssets::mark_scanned( nc::StringView path, uint64_t source_hash, int64_t source_mtime, uint64_t source_size )
{
    auto* entry = find_mut( path );
    if (!entry)
        return;

    entry->source_hash  = source_hash;
    entry->source_mtime = source_mtime;
    entry->source_size  = source_size;

    if (!manifest.valid_header)
        manifest.load( asset_paths.manifest_path );

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

    manifest.ctx          = manifest_ctx( asset_paths );
    manifest.valid_header = true;
}

void EditorAssets::flush_manifest()
{
    if (!manifest_dirty)
        return;
    manifest_dirty = false;
    manifest.save( asset_paths.manifest_path );
}

//------------------------------------------------------------------------------
// Automatic source change detection for reliable hot reload
//------------------------------------------------------------------------------

void EditorAssets::poll_source_changes()
{
    if (!importer.ready() || assets.empty())
        return;

    // Throttle to avoid hammering the filesystem every single frame.
    // ~10 checks per second is plenty and feels instant.
    // We don't have direct TimeComponent here; use a simple counter instead.
    static int frame_counter = 0;
    ++frame_counter;
    if (( frame_counter % 6 ) != 0) // every ~6 frames at 60fps ~10Hz
        return;

    std::error_code ec;
    bool any_queued = false;

    for (auto& entry : assets) {
        if (entry.source_path.empty())
            continue;
        if (entry.status == AssetStatus::COMPILING || entry.status == AssetStatus::ERROR)
            continue;

        auto disk_mtime = std::filesystem::last_write_time( entry.source_path.c_str(), ec ).time_since_epoch().count();
        if (ec)
            continue;

        if (disk_mtime > entry.source_mtime) {
            // Source changed externally (user saved in VSCode, etc.)
            entry.source_mtime = disk_mtime; // update so we don't spam
            entry.status       = AssetStatus::DIRTY;
            importer.enqueue( entry.path );
            any_queued = true;
            NC_LOG_INFO_C( nc::log::IO, "Hot reload: source changed '{}'", entry.path.c_str() );
        }
    }

    if (any_queued) {
        // Manifest will be flushed on next successful import
    }
}

void EditorAssets::refresh_materials_using_shader( nc::StringView shader_asset_path )
{
    if (shader_asset_path.empty())
        return;

    const nc::String target_path( shader_asset_path.data(), shader_asset_path.size() );

    // Get the fresh canonical shader after the import.
    nc::Ref<nc::MaterialShader> fresh = load<nc::MaterialShader>( target_path );
    if (!fresh)
        return;

    // Walk the scene and update any Material that is currently using a shader with the same filepath.
    // This keeps live editing + simulation working without requiring the user to re-assign anything.
    nc::Node* root = scene.root();
    if (!root)
        return;

    std::function<void( nc::Node& )> walk = [&]( nc::Node& node ) {
        // Check MaterialOverrideComponent
        if (auto* ov = node.get_component<nc::MaterialOverrideComponent>()) {
            if (ov->material) {
                auto sh = ov->material->get_shader();
                if (sh && sh->filepath == target_path) {
                    ov->material->set_shader( fresh );
                    // Force dirty so uniforms get re-uploaded next frame
                    // (the material will detect layout change via revision inside set_shader path)
                }
            }
        }

        // Also check direct MeshInstance materials? (rare, most use override or asset mesh material)
        if (auto* mi = node.get_component<nc::MeshInstanceComponent>()) {
            if (mi->mesh) {
                // Mesh itself may hold a material; if the mesh asset was not reloaded,
                // its internal material ref may still be old. We leave mesh asset reload
                // to the normal load(true) path.
            }
        }

        for (auto& child : node.get_children())
            walk( child );
    };

    walk( *root );
}

void EditorAssets::mark_failed( nc::StringView path, nc::StringView error )
{
    if (auto* entry = find_mut( path )) {
        entry->status     = AssetStatus::ERROR;
        entry->last_error = nc::String( error.data(), error.size() );
    }
}

void EditorAssets::rebuild_index()
{
    std::sort( assets.begin(), assets.end(), []( const AssetEntry& a, const AssetEntry& b ) {
        return a.path < b.path;
    } );
    by_path.clear();
    for (size_t i = 0; i < assets.size(); ++i)
        by_path.emplace( assets[i].path, static_cast<uint32_t>( i ) );
}

void EditorAssets::on_import_started( nc::StringView p_path )
{
    mark_compiling( p_path );
}

void EditorAssets::on_import_finished( const AssetImportResult& p_result )
{
    if (p_result.hash_only) {
        if (p_result.ok)
            mark_scanned( p_result.path, p_result.source_hash, p_result.source_mtime, p_result.source_size );
        else
            mark_failed( p_result.path, p_result.message );
        return;
    }

    if (p_result.ok) {
        scene.get_resource_loader().load( p_result.path, true );
        mark_imported( p_result.path, p_result.source_hash, p_result.source_mtime, p_result.source_size );

        // For shaders: make sure any live Materials pointing at the old shader instance get the fresh one.
        // This is what makes "edit .slang, save, see changes immediately while sim runs" robust.
        const AssetEntry* e = find( p_result.path );
        if (p_result.path.ends_with( ".slang" ) ||
            ( e && ( e->kind == AssetKind::MATERIAL_SHADER || e->kind == AssetKind::SHADER ) )) {
            refresh_materials_using_shader( p_result.path );
        }
    } else {
        mark_failed( p_result.path, p_result.message );
    }
}

} // namespace sea::editor
