#include "stb_image_loader.h"

#include <stb_image.h>

#include <ncore/resources/image.h>
#include <ncore/utils/log.h>

namespace nc {

bool StbImageLoader::is_handling_extension( const String& ext )
{
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".gif" ||
           ext == ".hdr" || ext == ".psd" || ext == ".pic" || ext == ".pgm" || ext == ".ppm";
}

Ref<IResource> StbImageLoader::import( const String& path, Context ctx )
{
    int width       = 0;
    int height      = 0;
    int channels    = 0;
    stbi_uc* pixels = stbi_load( path.data(), &width, &height, &channels, 4 );

    if (!pixels) {
        NC_LOG_ERROR_C( log::IO, "Failed to import image from path: {}. Reason: {}", path, stbi_failure_reason() );
        return {};
    }

    auto result = Ref<Image>::create( width, height, pixels );
    stbi_image_free( pixels );

    NC_LOG_DEBUG_C( log::IO, "Imported image '{}': {}x{} (channels={})", path, width, height, channels );

    return result.as<IResource>();
}

} // namespace nc
