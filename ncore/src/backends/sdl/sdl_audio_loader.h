#pragma once

#include <string>

#include <ncore/modules/resource/resource_importer.h>

namespace nc {

class SDLAudioLoader : public IResourceImporter {
    NCLASS( SDLAudioLoader, IResourceImporter )

public:
    bool is_handling_extension( const std::string& ext ) override;

    Ref<IResource> import( const std::string_view path ) override;
};

} // namespace nc
