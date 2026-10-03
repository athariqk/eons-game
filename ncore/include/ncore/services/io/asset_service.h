// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/object.h>
#include <ncore/services/service.h>

#include "asset_compiler.h"

namespace nc {

/**
 * @brief AssetService provides asset compilation services.
 *
 * Raw assets are imported/compiled via IAssetCompilers and writes compiled binary
 * .res files fors runtime loading by ResourceLoader.
 */
class NCAPI AssetService : public IService {
    NCLASS( AssetService, IService )

    static constexpr int MAX_COMPILERS = 64;

public:
    AssetService();

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    void register_compiler( Ptr<IAssetCompiler>&& p_compiler );

    /**
     * @brief Compile a raw asset into a binary .res file.
     *
     * Reads the raw asset at p_input_file, finds the appropriate IAssetCompiler,
     * and executes the compiler.
     *
     * @param p_input_file Absolute path, or a path relative to `assets/`.
     * @param p_output_path Absolute path, or a path relative to `assets/`.
     * @param p_search_paths `#include` roots for the compiler (see
     *        AssetImportContext::search_paths). Empty uses the compiler default.
     * @return Error status.
     */
    Error compile(
        const String& p_input_file, const String& p_output_path,
        const DynamicArray<String>& p_search_paths = {}
    );

    template<std::derived_from<IAssetCompiler> T, typename... TArgs>
    void register_compiler( TArgs&&... args )
    {
        register_compiler( std::make_unique<T>( std::forward<TArgs>( args )... ) );
    }

private:
    int num_compilers = 0;
    Array<Ptr<IAssetCompiler>, MAX_COMPILERS> compilers;
};

} // namespace nc
