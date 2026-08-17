#include <ncore/resources/material_template.h>

namespace nc {

ResourceFormatID MaterialTemplate::get_format_id() const
{
    return "matl";
}

size_t MaterialTemplate::get_size_bytes() const
{
    return 0;
}

} // namespace nc
