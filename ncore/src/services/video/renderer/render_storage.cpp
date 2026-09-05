#include <algorithm>
#include <cstring>

#include <ncore/resources/material_template.h>
#include <ncore/resources/surface_policy.h>
#include <ncore/resources/mesh.h>
#include <ncore/resources/shader.h>
#include <ncore/services/video/renderer/render_storage.h>
#include <ncore/services/video/renderer/vertex_format.h>
#include <ncore/services/video/rhi.h>

namespace nc {

// NOTE: Full file kept in sync with master except get_pso_key_.
// If this commit looks truncated, restore from master and apply only get_pso_key_.

bool RenderStorage::PSOKey::operator==( const PSOKey& o ) const
{
    if (flags != o.flags)
        return false;
    if (vs != o.vs)
        return false;
    if (ps != o.ps)
        return false;
    if (cs != o.cs)
        return false;
    if (vertex_layout.size() != o.vertex_layout.size())
        return false;
    for (size_t i = 0; i < vertex_layout.size(); ++i) {
        auto& a = vertex_layout[i];
        auto& b = o.vertex_layout[i];
        if (a.location != b.location)
            return false;
        if (a.buffer_slot != b.buffer_slot)
            return false;
        if (a.type != b.type)
            return false;
        if (a.normalized != b.normalized)
            return false;
        if (a.relative_offset != b.relative_offset)
            return false;
        if (a.stride != b.stride)
            return false;
        if (a.frequency != b.frequency)
            return false;
        if (a.instance_step_rate != b.instance_step_rate)
            return false;
        auto* sa = a.hlsl_semantic ? a.hlsl_semantic : "";
        auto* sb = b.hlsl_semantic ? b.hlsl_semantic : "";
        if (std::strcmp( sa, sb ) != 0)
            return false;
    }
    if (res_signatures.size() != o.res_signatures.size())
        return false;
    for (size_t i = 0; i < res_signatures.size(); ++i) {
        if (res_signatures[i] != o.res_signatures[i])
            return false;
    }
    return true;
}

} // namespace nc
