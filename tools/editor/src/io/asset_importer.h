#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/types.h>

namespace nc {
class AssetService;
class ResourceLoader;
struct AppContext;
} // namespace nc

namespace nc::editor {

struct AssetImportResult {
    bool Ok = false;
    String Message;
    double Ms = 0.0;
};

/**
 * @brief Recompiles a raw asset and reloads the compiled result.
 *
 * Compiles through AssetService into "<asset>.bin.tmp", replaces the runtime
 * ".bin" on success, then calls ResourceLoader::load(path, skip_cache=true) so
 * live components rebind through the usual ResourceLoaded path.
 */
class AssetImporter {
public:
    void init( const AppContext& app, ResourceLoader& loader, AssetService& assets );

    bool ready() const
    {
        return Assets != nullptr;
    }

    AssetImportResult import( StringView asset_path );

    const AssetImportResult* last_result( StringView asset_path ) const [[clang::lifetimebound]];

    /**
     * @brief True when the raw source is newer than the compiled .bin.
     */
    bool source_newer( StringView asset_path ) const;

private:
    bool find_source( StringView asset_path, String& out_path ) const;

    AssetService* Assets   = nullptr;
    ResourceLoader* Loader = nullptr;
    DynamicArray<String> SourceDirs;
    DynamicArray<String> SearchPaths;
    HashMap<String, AssetImportResult> Results;
};

} // namespace nc::editor
