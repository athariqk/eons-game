#include <ncore/modules/video/resources/material.h>

namespace nc {

std::span<Shader> Material::get_shaders()
{
    return shaders;
}

std::span<const Shader> Material::get_shaders() const
{
    return shaders;
}

} // namespace nc
