#pragma once

#include <ncore/services/io/asset_compiler.h>

namespace nc {

class StbImageCompiler : public IAssetCompiler {
    NCLASS( StbImageCompiler, IAssetCompiler )

public:
    StringView get_format_name() const override;
    bool is_handling_extension( const String& ext ) override;

    Error compile( StringView p_input, StringView p_output, AssetImportContext ctx ) override;
};

} // namespace nc
