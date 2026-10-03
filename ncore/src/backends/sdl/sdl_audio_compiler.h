#pragma once

#include <ncore/services/io/asset_compiler.h>

namespace nc {

class SDLAudioCompiler : public IAssetCompiler {
    NCLASS( SDLAudioCompiler, IAssetCompiler )

public:
    StringView get_format_name() const override;
    bool is_handling_extension( const String& ext ) override;

    Error compile( StringView p_input, StringView p_output, AssetImportContext p_ctx ) override;
};

} // namespace nc
