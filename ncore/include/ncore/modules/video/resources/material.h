#pragma once

#include <ncore/kernel/collection.h>
#include <ncore/modules/resource/resource.h>
#include <ncore/modules/video/resources/shader.h>

namespace nc {

class Material : public IResource {
    NCLASS( Material, IResource )

public:
    static const int MAX_SHADERS = 8;

public:
    std::span<Shader> get_shaders();
    std::span<const Shader> get_shaders() const;

private:
    Array<Shader, MAX_SHADERS> shaders;
};

} // namespace nc
