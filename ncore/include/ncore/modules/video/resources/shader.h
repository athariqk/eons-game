#pragma once

#include <ncore/modules/resource/resource.h>

namespace nc {

class Shader : public IResource {
    NCLASS( Shader, IResource )

public:
    std::string_view get_source() const
    {
        return source;
    }

private:
    std::string source;
};

} // namespace nc
