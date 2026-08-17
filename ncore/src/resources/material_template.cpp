#include <ncore/resources/material_template.h>

namespace nc {

ResourceFormatID MaterialTemplate::get_format_id() const
{
    return "matl";
}

size_t MaterialTemplate::get_size_bytes() const
{
    size_t total = 0;
    if (vs)
        total += vs->get_size_bytes();
    if (ps)
        total += ps->get_size_bytes();
    total += params.size() * sizeof( ShaderParamInfo );
    total += textures.size() * sizeof( MaterialTextureSlot );
    return total;
}

} // namespace nc
