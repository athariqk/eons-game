#pragma once

#include <string>

#include <ncore/modules/resource/resource_importer.h>

namespace nc {

class SDLImageLoader : public IResourceImporter {
    NCLASS( SDLImageLoader, IResourceImporter )

public:
    bool is_handling_extension( const std::string& ext ) override;

    Ref<IResource> import( const std::string_view path ) override;
};

} // namespace nc
