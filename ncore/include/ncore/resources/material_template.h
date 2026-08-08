#pragma once

#include <ncore/resources/resource.h>
#include <ncore/services/video/rhi_types.h>

#include "shader.h"

namespace nc {

struct NCAPI MaterialTextureSlot {
    std::string name;
    uint32_t binding;
};

class NCAPI MaterialTemplate : public IResource {
    NCLASS( MaterialTemplate, IResource )

public:
    ResourceFormatID get_format_id() const override;

    std::string debug_name;
    std::string vertex_layout_name;

    Ref<Shader> vs;
    Ref<Shader> ps;

    CullMode cull_mode                     = CullMode::NONE;
    bool depth_test                        = false;
    bool depth_write                       = false;
    BlendPreset blend                      = BlendPreset::ALPHA_BLEND;
    MultisampleStateDesc multisample_state = { 1, 0 };

    DynamicArray<ShaderParamInfo> params;
    DynamicArray<MaterialTextureSlot> textures;
};

} // namespace nc
