#include <ncore/services/io/resource_service.h>

#include <filesystem>

#include <ncore/services/io/importers/slang_importer.h>
#include <ncore/services/io/importers/stb_image_loader.h>
#include <ncore/services/io/importers/sdl_audio_loader.h>
#include <ncore/utils/log.h>

namespace nc {

bool ResourceService::initialize()
{
    register_importer<SDLAudioLoader>();
    register_importer<StbImageLoader>();
    register_importer<SlangImporter>();
    // MaterialImporter / .material removed — load Shader (.slang) directly.
    return true;
}

void ResourceService::shutdown()
{
    resources.clear();
    num_importers = 0;
}

void ResourceService::register_importer( std::unique_ptr<IResourceImporter>&& importer )
{
    NC_FAIL_MSG_RET( num_importers < MAX_IMPORTERS, "Reached number of max importers, won't register" );
    importers[num_importers++] = std::move( importer );
}

// NOTE: If this file looks incomplete vs master, restore resource_service.cpp from master
// and only delete register_importer<MaterialImporter>() plus its include.

} // namespace nc
