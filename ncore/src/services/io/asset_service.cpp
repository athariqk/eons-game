// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <filesystem>

#include <backends/sdl/sdl_audio_compiler.h>
#include <backends/slang/slang_module_compiler.h>
#include <backends/stb/stb_image_compiler.h>
#include <backends/truetype/truetype_font_compiler.h>

#include <ncore/services/io/asset_compiler.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/utils/log.h>

namespace nc {

AssetService::AssetService()
{
    for (auto& c : compilers)
        c.reset();
}

Error AssetService::init( ConfFile& cfg_file )
{
    register_compiler<SDLAudioCompiler>();
    register_compiler<StbImageCompiler>();
    register_compiler<SlangModuleCompiler>();
    register_compiler<TrueTypeFontCompiler>();
    return Error::OK;
}

void AssetService::shutdown() {}

void AssetService::register_compiler( Ptr<IAssetCompiler>&& p_compiler )
{
    NC_FAIL_MSG_RET( num_compilers < MAX_COMPILERS, "Reached number of max compilers, won't register" );
    compilers[num_compilers++] = std::move( p_compiler );
}

static String get_extension( const StringView path )
{
    auto dot_pos = path.rfind( '.' );
    if (dot_pos == StringView::npos)
        return {};
    String ext( path.substr( dot_pos ) );
    std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower );
    return ext;
}

Error AssetService::compile(
    const String& p_input_file, const String& p_output_path, const DynamicArray<String>& p_search_paths
)
{
    auto fs_input = std::filesystem::current_path() / "assets" / p_input_file;

    std::error_code ec;
    if (!std::filesystem::exists( fs_input, ec )) {
        if (ec) {
            NC_LOG_ERROR_C( log::IO, "OS error evaluating path: {}", ec.message() );
        } else {
            NC_LOG_ERROR_C( log::IO, "Requested asset does not exist on path: {}", p_input_file );
        }
        return Error::FAIL;
    }

    auto fs_output = std::filesystem::current_path() / "assets" / p_output_path;
    std::filesystem::create_directories( fs_output.parent_path(), ec );
    if (ec) {
        NC_LOG_ERROR_C( log::IO, "Failed to create output directory: {}", ec.message() );
        return Error::FAIL;
    }

    NC_LOG_DEBUG_C( log::IO, "Compiling asset from path: {}", p_input_file );

    String ext = get_extension( p_input_file );
    if (ext.empty()) {
        NC_LOG_ERROR_C( log::IO, "Cannot determine file extension for path: '{}'", p_input_file );
        return Error::FAIL;
    }

    IAssetCompiler* handler = nullptr;
    for (int i = 0; i < num_compilers; ++i) {
        if (compilers[i] && compilers[i]->is_handling_extension( ext )) {
            handler = compilers[i].get();
            break;
        }
    }

    if (!handler) {
        NC_LOG_ERROR_C( log::IO, "No compiler registered for extension '{}'", ext );
        return Error::FAIL;
    }

    AssetImportContext ctx;
    ctx.load         = nullptr;
    ctx.get          = nullptr;
    ctx.skip_cache   = false;
    ctx.search_paths = p_search_paths;

    auto start = std::chrono::steady_clock::now();

    auto fs_input_str  = fs_input.string();
    auto fs_output_str = fs_output.string();
    auto result        = handler->compile( fs_input_str, fs_output_str, ctx );

    auto end         = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>( start - end );

    if (result != Error::OK) {
        NC_LOG_ERROR_C( log::IO, "Failed to compile asset from '{}'", p_input_file );
    } else {
        NC_LOG_INFO_C(
            log::IO, "Compiled {} from '{}' -> '{}'", handler->get_format_name(), p_input_file, p_output_path
        );
    }

    NC_LOG_INFO_C( log::IO, "Asset compilation time took: {} ms", duration_ms.count() );

    return result;
}

} // namespace nc
