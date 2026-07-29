#pragma once

#include <ncore/modules/video/rhi_types.h>
#include <ncore/resources/resource.h>

namespace nc {

struct ShaderParamField {
    std::string name;
    uint32_t offset;
    uint32_t size;
    size_t stride;
    ShaderValueType type;
};

struct ShaderParamInfo {
    std::string name;
    std::string semantic_name;
    ResourceType resource_type = ResourceType::CONSTANT_BUFFER;
    ShaderValueType value_type = ShaderValueType::UNKNOWN;
    uint32_t binding_idx       = 0;
    uint32_t binding_space     = 0;
    uint32_t location          = 0;
    size_t offset              = 0;
    size_t stride              = 0;
    DynArray<ShaderParamField> fields;
};

using ShaderParamLayout = DynArray<ShaderParamInfo>;

struct ShaderDesc {
    ShaderType stage;
    std::string entrypoint;
    DynArray<uint32_t> bytecode;
    ShaderParamLayout params;
    VertexLayout vert_layout;
};

class Shader : public IResource {
    NCLASS( Shader, IResource )

public:
    Shader( const ShaderDesc& p_desc );

    ResourceFormatID get_format_id() override;

    ShaderType get_stage() const;
    std::string_view get_entry_point() const;
    std::span<const uint32_t> get_bytecode() const;
    const ShaderDesc& get_desc() const;

private:
    ShaderDesc desc;
};

class CompositeShader : public IResource {
    NCLASS( CompositeShader, IResource )

public:
    Ref<Shader> get_shader( ShaderType stage );
    void set_shader( ShaderType stage, const Ref<Shader>& shader );

private:
    HashMap<ShaderType, Ref<Shader>> shaders;
};

} // namespace nc
