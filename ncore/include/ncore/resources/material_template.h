#pragma once

/**
 * Thin material asset: points at a Shader. Pipeline state is NOT authored here.
 *
 * Surface defaults  -> Shader (// nc_pipeline: / SurfacePolicy)
 * Instance overrides -> MaterialComponent / material_set_draw_mode
 * Pass depth/RT     -> PassPipelineDefaults at PSO build time
 */

#include <ncore/resources/resource.h>
#include <ncore/resources/shader.h>

namespace nc {

class NCAPI MaterialTemplate : public IResource {
    NCLASS( MaterialTemplate, IResource )

public:
    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    String debug_name;

    /** Optional explicit mesh vertex layout name (e.g. Vertex3D). Empty = reflect from VS. */
    String vertex_layout_name;

    Ref<Shader> shader;
};

} // namespace nc
