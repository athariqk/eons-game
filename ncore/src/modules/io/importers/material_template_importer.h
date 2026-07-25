#pragma once

#include <ncore/modules/io/resource_importer.h>

namespace nc {

class ResourceManager;

class MaterialImporter : public IResourceImporter {
    NCLASS( MaterialImporter, IResourceImporter )

public:
    explicit MaterialImporter( ResourceManager* rm );

    bool is_handling_extension( const std::string& ext ) override
    {
        return ext == ".material";
    }

    Ref<IResource> import( std::string_view path ) override;

private:
    ResourceManager* resource_manager;
};

} // namespace nc
