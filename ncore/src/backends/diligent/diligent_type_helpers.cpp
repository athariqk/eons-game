#include "diligent_type_helpers.h"

namespace nc {

Diligent::TEXTURE_FORMAT DiligentTypeHelpers::translate_tex_format( rhi::TextureFormat format )
{
    switch (format) {
        case rhi::TextureFormat::D32_FLOAT:
            return Diligent::TEX_FORMAT_D32_FLOAT;
        case rhi::TextureFormat::RGBA8_UNORM:
            return Diligent::TEX_FORMAT_RGBA8_UNORM;
        case rhi::TextureFormat::RGBA8_UNORM_SRGB:
            return Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
        case rhi::TextureFormat::R32_FLOAT:
            return Diligent::TEX_FORMAT_R32_FLOAT;
        case rhi::TextureFormat::RG32_FLOAT:
            return Diligent::TEX_FORMAT_RG32_FLOAT;
        case rhi::TextureFormat::RGBA32_FLOAT:
            return Diligent::TEX_FORMAT_RGBA32_FLOAT;
        case rhi::TextureFormat::UNKNOWN:
            return Diligent::TEX_FORMAT_UNKNOWN;
    }
    NC_ASSERT( false, "Unhandled rhi::TextureFormat" );
    return Diligent::TEX_FORMAT_UNKNOWN;
}

Diligent::PRIMITIVE_TOPOLOGY DiligentTypeHelpers::translate_prim_topology( rhi::PrimitiveTopology topology )
{
    switch (topology) {
        case rhi::PrimitiveTopology::TRIANGLE_LIST:
            return Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case rhi::PrimitiveTopology::POINT_LIST:
            return Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    NC_ASSERT( false, "Unhandled rhi::PrimitiveTopology" );
    return Diligent::PRIMITIVE_TOPOLOGY_UNDEFINED;
}

Diligent::CULL_MODE DiligentTypeHelpers::translate_cull( rhi::CullMode c )
{
    switch (c) {
        case rhi::CullMode::NONE:
            return Diligent::CULL_MODE_NONE;
        case rhi::CullMode::FRONT:
            return Diligent::CULL_MODE_FRONT;
        case rhi::CullMode::BACK:
            return Diligent::CULL_MODE_BACK;
    }
    NC_ASSERT( false, "Unhandled rhi::CullMode" );
    return Diligent::CULL_MODE_NONE;
}

Diligent::COMPARISON_FUNCTION DiligentTypeHelpers::translate_comp_func( rhi::CompareFunc c )
{
    switch (c) {
        case rhi::CompareFunc::NEVER:
            return Diligent::COMPARISON_FUNC_NEVER;
        case rhi::CompareFunc::LESS:
            return Diligent::COMPARISON_FUNC_LESS;
        case rhi::CompareFunc::EQUAL:
            return Diligent::COMPARISON_FUNC_EQUAL;
        case rhi::CompareFunc::LESS_EQUAL:
            return Diligent::COMPARISON_FUNC_LESS_EQUAL;
        case rhi::CompareFunc::GREATER:
            return Diligent::COMPARISON_FUNC_GREATER;
        case rhi::CompareFunc::NOT_EQUAL:
            return Diligent::COMPARISON_FUNC_NOT_EQUAL;
        case rhi::CompareFunc::GREATER_EQUAL:
            return Diligent::COMPARISON_FUNC_GREATER_EQUAL;
        case rhi::CompareFunc::ALWAYS:
            return Diligent::COMPARISON_FUNC_ALWAYS;
    }
    NC_ASSERT( false, "Unhandled rhi::CompareFunc" );
    return Diligent::COMPARISON_FUNC_UNKNOWN;
}

Diligent::STENCIL_OP DiligentTypeHelpers::translate_stencil_op( rhi::StencilOp op )
{
    switch (op) {
        case rhi::StencilOp::KEEP:
            return Diligent::STENCIL_OP_KEEP;
        case rhi::StencilOp::ZERO:
            return Diligent::STENCIL_OP_ZERO;
        case rhi::StencilOp::REPLACE:
            return Diligent::STENCIL_OP_REPLACE;
        case rhi::StencilOp::INCR_CLAMP:
            return Diligent::STENCIL_OP_INCR_SAT;
        case rhi::StencilOp::DECR_CLAMP:
            return Diligent::STENCIL_OP_DECR_SAT;
        case rhi::StencilOp::INVERT:
            return Diligent::STENCIL_OP_INVERT;
        case rhi::StencilOp::INCR_WRAP:
            return Diligent::STENCIL_OP_INCR_WRAP;
        case rhi::StencilOp::DECR_WRAP:
            return Diligent::STENCIL_OP_DECR_WRAP;
    }
    NC_ASSERT( false, "Unhandled rhi::StencilOp" );
    return Diligent::STENCIL_OP_UNDEFINED;
}

Diligent::BLEND_OPERATION DiligentTypeHelpers::translate_blend_op( rhi::BlendOp op )
{
    switch (op) {
        case rhi::BlendOp::ADD:
            return Diligent::BLEND_OPERATION_ADD;
        case rhi::BlendOp::SUBTRACT:
            return Diligent::BLEND_OPERATION_SUBTRACT;
        case rhi::BlendOp::REV_SUBTRACT:
            return Diligent::BLEND_OPERATION_REV_SUBTRACT;
        case rhi::BlendOp::MIN:
            return Diligent::BLEND_OPERATION_MIN;
        case rhi::BlendOp::MAX:
            return Diligent::BLEND_OPERATION_MAX;
    }
    NC_ASSERT( false, "Unhandled rhi::BlendOp" );
    return Diligent::BLEND_OPERATION_UNDEFINED;
}

Diligent::BLEND_FACTOR DiligentTypeHelpers::translate_blend_factor( rhi::BlendFactor factor )
{
    switch (factor) {
        case nc::rhi::BlendFactor::ZERO:
            return Diligent::BLEND_FACTOR_ZERO;
        case nc::rhi::BlendFactor::ONE:
            return Diligent::BLEND_FACTOR_ONE;
        case nc::rhi::BlendFactor::SRC_COLOR:
            return Diligent::BLEND_FACTOR_SRC_COLOR;
        case nc::rhi::BlendFactor::INV_SRC_COLOR:
            return Diligent::BLEND_FACTOR_INV_SRC_COLOR;
        case nc::rhi::BlendFactor::SRC_ALPHA:
            return Diligent::BLEND_FACTOR_SRC_ALPHA;
        case nc::rhi::BlendFactor::INV_SRC_ALPHA:
            return Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        case nc::rhi::BlendFactor::DST_COLOR:
            return Diligent::BLEND_FACTOR_DEST_COLOR;
        case nc::rhi::BlendFactor::INV_DST_COLOR:
            return Diligent::BLEND_FACTOR_INV_DEST_COLOR;
        case nc::rhi::BlendFactor::DST_ALPHA:
            return Diligent::BLEND_FACTOR_DEST_ALPHA;
        case nc::rhi::BlendFactor::INV_DST_ALPHA:
            return Diligent::BLEND_FACTOR_INV_DEST_ALPHA;
        case nc::rhi::BlendFactor::CONSTANT_COLOR:
            return Diligent::BLEND_FACTOR_BLEND_FACTOR;
    }
    NC_ASSERT( false, "Unhandled rhi::BlendFactor" );
    return Diligent::BLEND_FACTOR_UNDEFINED;
}

Diligent::RESOURCE_DIMENSION DiligentTypeHelpers::translate_resource_dim( rhi::ResourceDimension dim )
{
    switch (dim) {
        case rhi::ResourceDimension::DIM_1D:
            return Diligent::RESOURCE_DIM_TEX_1D;
        case rhi::ResourceDimension::DIM_2D:
            return Diligent::RESOURCE_DIM_TEX_2D;
        case rhi::ResourceDimension::DIM_3D:
            return Diligent::RESOURCE_DIM_TEX_3D;
        case rhi::ResourceDimension::DIM_CUBE:
            return Diligent::RESOURCE_DIM_TEX_CUBE;
    }
    NC_ASSERT( false, "Unhandled rhi::ResourceDimension" );
    return Diligent::RESOURCE_DIM_UNDEFINED;
}

void DiligentTypeHelpers::apply_depth_stencil_op( Diligent::StencilOpDesc& to, const rhi::StencilOpDesc& from )
{
    Diligent::StencilOpDesc result{};
    result.StencilDepthFailOp = translate_stencil_op( from.depth_fail );
    result.StencilFailOp      = translate_stencil_op( from.fail );
    result.StencilPassOp      = translate_stencil_op( from.pass );
    result.StencilFunc        = translate_comp_func( from.func );
}

void DiligentTypeHelpers::apply_depth_stencil_state(
    Diligent::DepthStencilStateDesc& to, const rhi::DepthStencilStateDesc& from
)
{
    apply_depth_stencil_op( to.BackFace, from.back );
    apply_depth_stencil_op( to.FrontFace, from.front );
}

Diligent::FILL_MODE DiligentTypeHelpers::translate_fill_mode( rhi::FillMode mode )
{
    switch (mode) {
        case rhi::FillMode::SOLID:
            return Diligent::FILL_MODE_SOLID;
        case rhi::FillMode::WIREFRAME:
            return Diligent::FILL_MODE_WIREFRAME;
    }
    NC_ASSERT( false, "Unhandled rhi::FillMode" );
    return Diligent::FILL_MODE_UNDEFINED;
}

Diligent::VALUE_TYPE DiligentTypeHelpers::translate_value_type( rhi::ShaderValueType type )
{
    switch (type) {
        case rhi::ShaderValueType::FLOAT:
        case rhi::ShaderValueType::FLOAT2:
        case rhi::ShaderValueType::FLOAT3:
        case rhi::ShaderValueType::FLOAT4:
            return Diligent::VT_FLOAT32;
        case rhi::ShaderValueType::INT:
        case rhi::ShaderValueType::INT2:
        case rhi::ShaderValueType::INT3:
        case rhi::ShaderValueType::INT4:
            return Diligent::VT_INT32;
        case rhi::ShaderValueType::UBYTE4_NORM:
            return Diligent::VT_UINT8;
        case rhi::ShaderValueType::USHORT4:
            return Diligent::VT_UINT16;
        case rhi::ShaderValueType::BOOL:
            return Diligent::VT_INT32;
        case rhi::ShaderValueType::MAT4:
            break;
        case rhi::ShaderValueType::TEXTURE_2D:
        case rhi::ShaderValueType::TEXTURE_CUBED:
        case rhi::ShaderValueType::SAMPLER:
        case rhi::ShaderValueType::UNKNOWN:
            break;
    }
    NC_ASSERT( false, "Unhandled rhi::ValueType" );
    return Diligent::VT_UNDEFINED;
}

uint32_t DiligentTypeHelpers::translate_value_num_components( rhi::ShaderValueType type )
{
    switch (type) {
        case rhi::ShaderValueType::FLOAT:
            return 1;
        case rhi::ShaderValueType::FLOAT2:
            return 2;
        case rhi::ShaderValueType::FLOAT3:
            return 3;
        case rhi::ShaderValueType::FLOAT4:
            return 4;
        case rhi::ShaderValueType::INT:
            return 1;
        case rhi::ShaderValueType::INT2:
            return 2;
        case rhi::ShaderValueType::INT3:
            return 3;
        case rhi::ShaderValueType::INT4:
            return 4;
        case rhi::ShaderValueType::UBYTE4_NORM:
            return 4;
        case rhi::ShaderValueType::USHORT4:
            return 4;
        case rhi::ShaderValueType::BOOL:
            return 1;
        case rhi::ShaderValueType::MAT4:
            break;
        case rhi::ShaderValueType::TEXTURE_2D:
        case rhi::ShaderValueType::TEXTURE_CUBED:
        case rhi::ShaderValueType::SAMPLER:
        case rhi::ShaderValueType::UNKNOWN:
            break;
    }
    NC_ASSERT( false, "Unhandled rhi::ValueType" );
    return 0;
}

Diligent::SHADER_TYPE DiligentTypeHelpers::translate_shader_stage( rhi::ShaderStage stage )
{
    switch (stage) {
        case rhi::ShaderStage::NONE:
            return Diligent::SHADER_TYPE_UNKNOWN;
        case rhi::ShaderStage::VERTEX:
            return Diligent::SHADER_TYPE_VERTEX;
        case rhi::ShaderStage::PIXEL:
            return Diligent::SHADER_TYPE_PIXEL;
        case rhi::ShaderStage::COMPUTE:
            return Diligent::SHADER_TYPE_COMPUTE;
        case rhi::ShaderStage::VS_PS:
            return Diligent::SHADER_TYPE_VS_PS;
    }
    NC_ASSERT( false, "Unhandled rhi::ShaderStage" );
    return Diligent::SHADER_TYPE_UNKNOWN;
}

Diligent::SHADER_RESOURCE_TYPE DiligentTypeHelpers::translate_resource_type( rhi::ResourceType type )
{
    switch (type) {
        case rhi::ResourceType::UNKNOWN:
            return Diligent::SHADER_RESOURCE_TYPE_UNKNOWN;
        case rhi::ResourceType::CONSTANT_BUFFER:
            return Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;
        case rhi::ResourceType::TEXTURE_SRV:
            return Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV;
        case rhi::ResourceType::BUFFER_SRV:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV;
        case rhi::ResourceType::TEXTURE_UAV:
            return Diligent::SHADER_RESOURCE_TYPE_TEXTURE_UAV;
        case rhi::ResourceType::BUFFER_UAV:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_UAV;
        case rhi::ResourceType::SAMPLER:
            return Diligent::SHADER_RESOURCE_TYPE_SAMPLER;
        case rhi::ResourceType::VARYING_INPUT:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV;
    }
    NC_ASSERT( false, "Unhandled rhi::ResourceType" );
    return Diligent::SHADER_RESOURCE_TYPE_UNKNOWN;
}

Diligent::SHADER_RESOURCE_VARIABLE_TYPE
DiligentTypeHelpers::translate_shader_resource_var_type( rhi::ResourceVarType type )
{
    switch (type) {
        case rhi::ResourceVarType::STATIC:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        case rhi::ResourceVarType::MUTABLE:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        case rhi::ResourceVarType::DYNAMIC:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
    }
    NC_ASSERT( false, "Unhandled rhi::ResourceVarType" );
    return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
}

Diligent::PIPELINE_RESOURCE_FLAGS DiligentTypeHelpers::translate_pipeline_resource_flags( rhi::ResourceFlags flags )
{
    switch (flags) {
        case rhi::ResourceFlags::NONE:
            return Diligent::PIPELINE_RESOURCE_FLAG_NONE;
        case rhi::ResourceFlags::NO_DYNAMIC_BUFFERS:
            return Diligent::PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS;
        case rhi::ResourceFlags::COMBINED_SAMPLER:
            return Diligent::PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;
        case rhi::ResourceFlags::FORMATTED_BUFFER:
            return Diligent::PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER;
    }
    NC_ASSERT( false, "Unhandled rhi::ResourceFlags" );
    return Diligent::PIPELINE_RESOURCE_FLAG_NONE;
}

Diligent::PipelineResourceDesc DiligentTypeHelpers::translate_resource_desc( const rhi::PipelineResourceDesc& from )
{
    Diligent::PipelineResourceDesc to{};
    to.Name         = from.name.c_str();
    to.ShaderStages = translate_shader_stage( from.stage );
    to.ResourceType = translate_resource_type( from.resource_type );
    to.VarType      = translate_shader_resource_var_type( from.var_type );
    to.Flags        = translate_pipeline_resource_flags( from.flags );
    to.ArraySize    = from.array_size;
    return to;
}

Diligent::INPUT_ELEMENT_FREQUENCY DiligentTypeHelpers::translate_vertex_frequency( rhi::VertexFrequency freq )
{
    switch (freq) {
        case rhi::VertexFrequency::PER_VERTEX:
            return Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
        case rhi::VertexFrequency::PER_INSTANCE:
            return Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;
    }
    NC_ASSERT( false, "Unhandled rhi::VertexFrequency" );
    return Diligent::INPUT_ELEMENT_FREQUENCY_UNDEFINED;
}

} // namespace nc
