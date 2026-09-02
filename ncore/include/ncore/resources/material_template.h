#pragma once

#include <ncore/resources/resource.h>
#include <ncore/services/video/rhi_types.h>

#include "shader.h"

namespace nc {

struct NCAPI MaterialTextureSlot {
    String name;
    uint32_t binding;
};

class NCAPI MaterialTemplate : public IResource {
    NCLASS( MaterialTemplate, IResource )

public:
    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    String debug_name;
    String vertex_layout_name;

    Ref<Shader> shader;

    rhi::CullMode cull_mode                     = rhi::CullMode::NONE;
    rhi::FillMode fill_mode                     = rhi::FillMode::SOLID;
    bool depth_test                             = false;
    bool depth_write                            = false;
    rhi::BlendPreset blend                      = rhi::BlendPreset::ALPHA_BLEND;
    rhi::MultisampleStateDesc multisample_state = { 1, 0 };

    // TODO: this is not used?
    DynamicArray<MaterialTextureSlot> textures;
};

} // namespace nc
