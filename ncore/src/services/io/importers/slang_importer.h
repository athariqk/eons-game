#pragma once

#include <ncore/services/io/resource_importer.h>

#include "../shader_compiler.h"

namespace nc {

class SlangImporter : public IResourceImporter {
    NCLASS( SlangImporter, IResourceImporter )

public:
    bool is_handling_extension( const std::string& ext ) override
    {
        return ext == ".slang";
    }

    Ref<IResource> import( std::string_view path, Context ctx ) override;

private:
    ShaderCompiler compiler;
};

} // namespace nc
