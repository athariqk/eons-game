#pragma once

#include <ncore/resources/resource.h>
#include <ncore/services/video/rhi_types.h>

namespace nc {

struct NCAPI ShaderParamField {
    String name;
    uint32_t offset;
    uint32_t size;
    size_t stride;
    ShaderValueType type;
};

struct NCAPI ShaderParamInfo {
    String name;
    String semantic_name;
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
    String entrypoint;
    DynamicArray<uint32_t> bytecode;
    ShaderParamLayout params;
    VertexLayout vert_layout;
    uint32_t num_threads_x = 1;
    uint32_t num_threads_y = 1;
    uint32_t num_threads_z = 1;
};

class NCAPI Shader : public IResource {
    NCLASS( Shader, IResource )

public:
    Shader( const ShaderDesc& p_desc );

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    ShaderType get_stage() const;
    StringView get_entry_point() const;
    Span<const uint32_t> get_bytecode() const;
    const ShaderDesc& get_desc() const;

private:
    ShaderDesc desc;
};

// TODO: merge into Shader
class NCAPI CompositeShader : public IResource {
    NCLASS( CompositeShader, IResource )

public:
    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    Ref<Shader> get_shader( ShaderType stage );
    void set_shader( ShaderType stage, const Ref<Shader>& shader );

private:
    HashMap<ShaderType, Ref<Shader>> shaders;
};

} // namespace nc
