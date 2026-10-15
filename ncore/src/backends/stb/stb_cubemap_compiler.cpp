#include "stb_cubemap_compiler.h"

#include <stb_image.h>

#include <ncore/resources/resource.h>
#include <ncore/resources/resource_archive.h>
#include <ncore/utils/log.h>

namespace nc {

StringView StbCubemapCompiler::get_format_name() const
{
    return "CubeMap";
}

bool StbCubemapCompiler::is_handling_extension( const String& ext )
{
    return ext == ".equirect";
}

Error StbCubemapCompiler::compile( StringView p_input, StringView p_output, AssetImportContext ctx )
{
    ( void )ctx;

    const String input_path( p_input );
    const String output_path( p_output );

    int      width    = 0;
    int      height   = 0;
    int      channels = 0;
    stbi_uc* pixels   = stbi_load( input_path.c_str(), &width, &height, &channels, 4 );
    if (!pixels) {
        NC_LOG_ERROR_C( log::IO, "Failed importing equirect image, reason: {}", stbi_failure_reason() );
        return Error::FAIL;
    }

    if (width < 4 || height < 2) {
        NC_LOG_ERROR_C( log::IO, "Equirect image too small: {}x{}", width, height );
        stbi_image_free( pixels );
        return Error::FAIL;
    }

    auto equirect = Ref<Image>::create();
    equirect->set_dimension( width, height );
    equirect->set_data( pixels );
    stbi_image_free( pixels );

    // Face size mirrors the previous runtime behavior: source width / 4.
    auto cubemap = Ref<CubeMap>::create( equirect, static_cast<uint32_t>( width ) / 4u );

    ResourceArchive archive;
    if (auto err = archive.serialize( cubemap, output_path ); err != Error::OK) {
        return err;
    }

    return Error::OK;
}

} // namespace nc
