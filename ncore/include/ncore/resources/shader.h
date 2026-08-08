#pragma once

#include <ncore/resources/resource.h>
#include <ncore/services/video/rhi_types.h>

namespace nc {

struct NCAPI ShaderParamField {
    std::string name;
    uint32_t offset;
    uint32_t size;
    size_t stride;
    ShaderValueType type;
};

struct NCAPI ShaderParamInfo {
    std::string name;
    std::string semantic_name;
    ResourceType resource_type = ResourceType::CONSTANT_BUFFER;
    ShaderValueType value_type = ShaderValueType::UNKNOWN;
    uint32_t binding_idx       = 0;
    uint32_t binding_space     = 0;
    uint32_t location          = 0;
    size_t offset              = 0;
    size_t stride              = 0;
    DynamicArray<ShaderParamField> fields;
};

using ShaderParamLayout = DynamicArray<ShaderParamInfo>;

struct NCAPI ShaderDesc {
    ShaderType stage;
    std::string entrypoint;
    DynamicArray<uint32_t> bytecode;
    ShaderParamLayout params;
    VertexLayout vert_layout;
};

class NCAPI Shader : public IResource {
    NCLASS( Shader, IResource )

public:
    Shader( const ShaderDesc& p_desc );

    ResourceFormatID get_format_id() const override;

    ShaderType get_stage() const;
    std::string_view get_entry_point() const;
    std::span<const uint32_t> get_bytecode() const;
    const ShaderDesc& get_desc() const;

private:
    ShaderDesc desc;
};

// TODO: merge into Shader
class CompositeShader : public IResource {
    NCLASS( CompositeShader, IResource )

public:
    ResourceFormatID get_format_id() const override;
    Ref<Shader> get_shader( ShaderType stage );
    void set_shader( ShaderType stage, const Ref<Shader>& shader );

private:
    HashMap<ShaderType, Ref<Shader>> shaders;
};

} // namespace nc
