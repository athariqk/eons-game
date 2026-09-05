#pragma once

/**
 * Material asset (Esoterica-style): shader + parameter values only.
 * Pipeline / blend / depth are NOT stored here — see MaterialShaderFlags and passes.
 */

#include <ncore/resources/material_shader.h>
#include <ncore/resources/resource.h>
#include <ncore/resources/shader.h>

namespace nc {

class NCAPI MaterialTemplate : public IResource {
    NCLASS( MaterialTemplate, IResource )

public:
    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    String debug_name;

    /** Optional mesh vertex layout name; empty = reflect from VS. */
    String vertex_layout_name;

    Ref<Shader> shader;

    /**
     * Surface batching flags. If None, derived from shader SurfacePolicy
     * (// nc_pipeline:). Importer may set from optional [flags] section.
     */
    MaterialShaderFlags flags = MaterialShaderFlags::None;

    /** Optional CPU defaults for material parameters (uploaded on create). */
    MaterialParameterStorage default_params{};
};

} // namespace nc
