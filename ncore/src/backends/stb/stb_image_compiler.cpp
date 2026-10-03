#include "stb_image_compiler.h"

#include <stb_image.h>

#include <ncore/resources/resource_archive.h>
#include <ncore/utils/log.h>

namespace nc {

StringView StbImageCompiler::get_format_name() const
{
    return "Image";
}

bool StbImageCompiler::is_handling_extension( const String& ext )
{
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".gif" ||
           ext == ".hdr" || ext == ".psd" || ext == ".pic" || ext == ".pgm" || ext == ".ppm";
}

Error StbImageCompiler::compile( StringView p_input, StringView p_output, AssetImportContext ctx )
{
    int width       = 0;
    int height      = 0;
    int channels    = 0;
    stbi_uc* pixels = stbi_load( p_input.data(), &width, &height, &channels, 4 );

    if (!pixels) {
        NC_LOG_ERROR_C( log::IO, "Failed importing image, reason: {}", stbi_failure_reason() );
        return Error::FAIL;
    }

    auto result = Ref<Image>::create();
    result->set_dimension( width, height );
    result->set_data( pixels );

    ResourceArchive archive;
    if (auto err = archive.serialize( result, p_output.data() ); err != Error::OK) {
        stbi_image_free( pixels );
        return err;
    }

    stbi_image_free( pixels );

    return Error::OK;
}

} // namespace nc
