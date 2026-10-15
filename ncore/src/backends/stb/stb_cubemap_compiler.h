#pragma once

#include <ncore/services/io/asset_compiler.h>

namespace nc {

/**
 * @brief Compiles an equirectangular panorama (.equirect) into a CubeMap resource.
 *
 * The expensive equirect -> 6-face conversion runs offline here, so the runtime only
 * deserializes and uploads the precomputed faces.
 */
class StbCubemapCompiler : public IAssetCompiler {
    NCLASS( StbCubemapCompiler, IAssetCompiler )

public:
    StringView get_format_name() const override;
    bool is_handling_extension( const String& ext ) override;

    Error compile( StringView p_input, StringView p_output, AssetImportContext ctx ) override;
};

} // namespace nc
