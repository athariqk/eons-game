#pragma once

#include <ncore/modules/video/rhi_types.h>
#include <ncore/resources/resource.h>

#include "shader.h"

namespace nc {

struct MaterialTextureSlot {
    std::string name;
    uint32_t binding;
};

class MaterialTemplate : public IResource {
    NCLASS( MaterialTemplate, IResource )

public:
    ResourceFormatID get_format_id() override;

    std::string debug_name;
    std::string vertex_layout_name;

    Ref<Shader> vs;
    Ref<Shader> ps;

    CullMode cull_mode                     = CullMode::NONE;
    bool depth_test                        = false;
    bool depth_write                       = false;
    BlendPreset blend                      = BlendPreset::ALPHA_BLEND;
    MultisampleStateDesc multisample_state = { 1, 0 };

    Vector<ShaderParamInfo> params;
    Vector<MaterialTextureSlot> textures;
};

} // namespace nc
