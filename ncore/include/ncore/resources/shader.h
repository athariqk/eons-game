#pragma once

#include <ncore/resources/resource.h>
#include <ncore/services/video/rhi_types.h>

namespace nc {

struct NCAPI ShaderParamField {
    String name;
    uint32_t offset;
    uint32_t size;
    size_t stride;
    rhi::ShaderValueType type;
};

struct NCAPI ShaderParamInfo {
    String name;
    String semantic_name;
    rhi::ResourceType resource_type = rhi::ResourceType::CONSTANT_BUFFER;
    rhi::ShaderValueType value_type = rhi::ShaderValueType::UNKNOWN;
    uint32_t binding_space          = 0;
    uint32_t binding_idx            = 0;
    uint32_t location               = 0;
    size_t offset                   = 0;
    size_t stride                   = 0;
    DynamicArray<ShaderParamField> fields;
    rhi::ShaderStage stage_mask = rhi::ShaderStage::NONE; // bitmask of all stages using this param.
};

using ShaderParamLayout = DynamicArray<ShaderParamInfo>;

/**
 * @brief Attributes of a shader program.
 */
struct NCAPI ShaderDesc {
    String name;
    rhi::ShaderStage stage;
    String entrypoint;
    DynamicArray<uint32_t> bytecode;
    ShaderParamLayout params;
    rhi::VertexLayout vert_layout;
    uint32_t num_threads_x = 1;
    uint32_t num_threads_y = 1;
    uint32_t num_threads_z = 1;
};

/**
 * @brief Shader resource represents compiled GPU program(s).
 */
class NCAPI Shader : public IResource {
    NCLASS( Shader, IResource )

public:
    /**
     * @brief Construct single-stage shader.
     */
    Shader( const ShaderDesc& p_desc );
    /**
     * @brief Construct multi-stage shader.
     */
    Shader( DynamicArray<ShaderDesc> p_stages_desc );

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    /**
     * @brief Get the shader's entrypoint name.
     */
    StringView get_entry_point( rhi::ShaderStage stage ) const;

    /**
     * @brief Get the shader's compiled bytecode.
     */
    Span<const uint32_t> get_bytecode( rhi::ShaderStage stage ) const;

    /**
     * @brief Return parameters from all available stages.
     */
    Span<const ShaderParamInfo> get_params() const;
    const ShaderParamInfo* find_param( String name ) const;

    bool has_stage( rhi::ShaderStage stage ) const;
    const ShaderDesc* get_stage_desc( rhi::ShaderStage stage ) const;

    /**
     * @brief Get each stage attributes making up this shader.
     */
    Span<const ShaderDesc> get_stages() const;

    rhi::ShaderStage get_stage_flags() const;

private:
    void build_configs_();

    DynamicArray<ShaderDesc> stages;
    ShaderParamLayout unified_params;
    HashMap<String, size_t> param_lookup;
};

} // namespace nc
