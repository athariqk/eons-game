#pragma once

#include <ncore/services/io/resource_importer.h>

namespace nc {

class ResourceService;

class MaterialImporter : public IResourceImporter {
    NCLASS( MaterialImporter, IResourceImporter )

public:
    bool is_handling_extension( const String& ext ) override
    {
        return ext == ".material";
    }

    Ref<IResource> import( const String& path, Context ctx ) override;
};

} // namespace nc
