#pragma once

#include <cstdint>
#include <filesystem>

#include <ncore/core/collection.h>

namespace nc {
struct AppContext;
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

/**
 * @brief Everything the importer and index share about where assets live.
 *
 * This is the single source of truth for path resolution: the raw source dirs
 * (pipeline order, later wins), the derived shader #include roots, the runtime
 * asset root and where the import manifest is persisted.
 */
struct AssetPaths {
    DynamicArray<String> source_dirs;  // raw asset roots, pipeline order (later wins)
    DynamicArray<String> search_paths; // derived shader #include roots (reversed)
    String asset_root;                 // runtime assets directory
    String manifest_path;              // where the import manifest lives
};

/**
 * @brief Resolve source dirs, shader search paths, asset root and manifest path.
 */
AssetPaths discover_asset_paths( const AppContext& app, const ResourceLoader& loader );

/**
 * @brief Resolve an asset path to its raw source (later source dir wins).
 */
bool find_source( StringView asset_path, const AssetPaths& paths, String& out );

/**
 * @brief Resolve one #include against the includer's dir then the search paths.
 *
 * Mirrors the Slang compiler's resolution order so the dependency hash matches
 * what actually gets compiled.
 */
bool resolve_include(
    StringView include, const std::filesystem::path& includer_dir, const AssetPaths& paths,
    std::filesystem::path& out );

/**
 * @brief FNV-1a over a byte string (seed defaults to the 64-bit offset basis).
 */
uint64_t fnv1a( StringView text, uint64_t seed = 1469598103934665603ull );

/**
 * @brief Hash a file's bytes (FNV-1a).
 */
uint64_t hash_file( const std::filesystem::path& p );

/**
 * @brief Hash a shader-like source together with every #include it reaches.
 */
uint64_t hash_source_tree( const std::filesystem::path& source, const AssetPaths& paths );

/**
 * @brief Whether this kind hashes its #include tree instead of just its bytes.
 */
bool kind_has_includes( AssetKind kind );

/**
 * @brief Read the FourCC resource-format id from a compiled .bin header.
 */
bool read_format_id( const std::filesystem::path& p, uint32_t& out );

/**
 * @brief Map a resource format FourCC to an asset kind.
 */
AssetKind kind_from_format( uint32_t id );

StringView asset_kind_name( AssetKind kind );

/**
 * @brief Whether a compiler exists for this kind (meshes have none yet).
 */
bool asset_kind_can_import( AssetKind kind );

/**
 * @brief Case-insensitive substring match used by the asset search boxes.
 */
bool asset_path_matches( StringView path, const char* filter );

} // namespace nc::editor
