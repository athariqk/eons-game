#pragma once

#include <ncore/services/io/resource_importer.h>

namespace nc {

class SDLAudioLoader : public IResourceImporter {
    NCLASS( SDLAudioLoader, IResourceImporter )

public:
    bool is_handling_extension( const String& ext ) override;

    Ref<IResource> import( const String& path, Context ctx ) override;
};

} // namespace nc
