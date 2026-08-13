#pragma once

#include <string>

#include <ncore/services/io/resource_importer.h>

namespace nc {

class StbImageLoader : public IResourceImporter {
    NCLASS( StbImageLoader, IResourceImporter )

public:
    bool is_handling_extension( const std::string& ext ) override;

    Ref<IResource> import( const std::string_view path, Context ctx ) override;
};

} // namespace nc
