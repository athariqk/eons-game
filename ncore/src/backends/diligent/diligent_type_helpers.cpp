#include "diligent_type_helpers.h"

namespace nc {

Diligent::TEXTURE_FORMAT DiligentTypeHelpers::translate_tex_format( TextureFormat format )
{
    switch (format) {
        case TextureFormat::D32_FLOAT:
            return Diligent::TEX_FORMAT_D32_FLOAT;
        case TextureFormat::RGBA8_UNORM:
            return Diligent::TEX_FORMAT_RGBA8_UNORM;
        case TextureFormat::RGBA8_UNORM_SRGB:
            return Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
        case TextureFormat::UNKNOWN:
            return Diligent::TEX_FORMAT_UNKNOWN;
    }
    NC_ASSERT( false, "Unhandled TextureFormat" );
    return Diligent::TEX_FORMAT_UNKNOWN;
}

Diligent::PRIMITIVE_TOPOLOGY DiligentTypeHelpers::translate_prim_topology( PrimitiveTopology topology )
{
    switch (topology) {
        case PrimitiveTopology::TRIANGLE_LIST:
            return Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::POINT_LIST:
            return Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    NC_ASSERT( false, "Unhandled Topology" );
    return Diligent::PRIMITIVE_TOPOLOGY_UNDEFINED;
}

Diligent::CULL_MODE DiligentTypeHelpers::translate_cull( CullMode c )
{
    switch (c) {
        case CullMode::NONE:
            return Diligent::CULL_MODE_NONE;
        case CullMode::FRONT:
            return Diligent::CULL_MODE_FRONT;
        case CullMode::BACK:
            return Diligent::CULL_MODE_BACK;
    }
    NC_ASSERT( false, "Unhandled CullMode" );
    return Diligent::CULL_MODE_NONE;
}

Diligent::COMPARISON_FUNCTION DiligentTypeHelpers::translate_comp_func( CompareFunc c )
{
    switch (c) {
        case CompareFunc::NEVER:
            return Diligent::COMPARISON_FUNC_NEVER;
        case CompareFunc::LESS:
            return Diligent::COMPARISON_FUNC_LESS;
        case CompareFunc::EQUAL:
            return Diligent::COMPARISON_FUNC_EQUAL;
        case CompareFunc::LESS_EQUAL:
            return Diligent::COMPARISON_FUNC_LESS_EQUAL;
        case CompareFunc::GREATER:
            return Diligent::COMPARISON_FUNC_GREATER;
        case CompareFunc::NOT_EQUAL:
            return Diligent::COMPARISON_FUNC_NOT_EQUAL;
        case CompareFunc::GREATER_EQUAL:
            return Diligent::COMPARISON_FUNC_GREATER_EQUAL;
        case CompareFunc::ALWAYS:
            return Diligent::COMPARISON_FUNC_ALWAYS;
    }
    NC_ASSERT( false, "Unhandled CompareFunc" );
    return Diligent::COMPARISON_FUNC_UNKNOWN;
}

Diligent::STENCIL_OP DiligentTypeHelpers::translate_stencil_op( StencilOp op )
{
    switch (op) {
        case StencilOp::KEEP:
            return Diligent::STENCIL_OP_KEEP;
        case StencilOp::ZERO:
            return Diligent::STENCIL_OP_ZERO;
        case StencilOp::REPLACE:
            return Diligent::STENCIL_OP_REPLACE;
        case StencilOp::INCR_CLAMP:
            return Diligent::STENCIL_OP_INCR_SAT;
        case StencilOp::DECR_CLAMP:
            return Diligent::STENCIL_OP_DECR_SAT;
        case StencilOp::INVERT:
            return Diligent::STENCIL_OP_INVERT;
        case StencilOp::INCR_WRAP:
            return Diligent::STENCIL_OP_INCR_WRAP;
        case StencilOp::DECR_WRAP:
            return Diligent::STENCIL_OP_DECR_WRAP;
    }
    NC_ASSERT( false, "Unhandled StencilOp" );
    return Diligent::STENCIL_OP_UNDEFINED;
}

Diligent::BLEND_OPERATION DiligentTypeHelpers::translate_blend_op( BlendOp op )
{
    switch (op) {
        case BlendOp::ADD:
            return Diligent::BLEND_OPERATION_ADD;
        case BlendOp::SUBTRACT:
            return Diligent::BLEND_OPERATION_SUBTRACT;
        case BlendOp::REV_SUBTRACT:
            return Diligent::BLEND_OPERATION_REV_SUBTRACT;
        case BlendOp::MIN:
            return Diligent::BLEND_OPERATION_MIN;
        case BlendOp::MAX:
            return Diligent::BLEND_OPERATION_MAX;
    }
    NC_ASSERT( false, "Unhandled BlendOp" );
    return Diligent::BLEND_OPERATION_UNDEFINED;
}

Diligent::BLEND_FACTOR DiligentTypeHelpers::translate_blend_factor( BlendFactor factor )
{
    switch (factor) {
        case nc::BlendFactor::ZERO:
            return Diligent::BLEND_FACTOR_ZERO;
        case nc::BlendFactor::ONE:
            return Diligent::BLEND_FACTOR_ONE;
        case nc::BlendFactor::SRC_COLOR:
            return Diligent::BLEND_FACTOR_SRC_COLOR;
        case nc::BlendFactor::INV_SRC_COLOR:
            return Diligent::BLEND_FACTOR_INV_SRC_COLOR;
        case nc::BlendFactor::SRC_ALPHA:
            return Diligent::BLEND_FACTOR_SRC_ALPHA;
        case nc::BlendFactor::INV_SRC_ALPHA:
            return Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        case nc::BlendFactor::DST_COLOR:
            return Diligent::BLEND_FACTOR_DEST_COLOR;
        case nc::BlendFactor::INV_DST_COLOR:
            return Diligent::BLEND_FACTOR_INV_DEST_COLOR;
        case nc::BlendFactor::DST_ALPHA:
            return Diligent::BLEND_FACTOR_DEST_ALPHA;
        case nc::BlendFactor::INV_DST_ALPHA:
            return Diligent::BLEND_FACTOR_INV_DEST_ALPHA;
        case nc::BlendFactor::CONSTANT_COLOR:
            return Diligent::BLEND_FACTOR_BLEND_FACTOR;
    }
    NC_ASSERT( false, "Unhandled BlendFactor" );
    return Diligent::BLEND_FACTOR_UNDEFINED;
}

Diligent::RESOURCE_DIMENSION DiligentTypeHelpers::translate_resource_dim( ResourceDimension dim )
{
    switch (dim) {
        case ResourceDimension::DIM_1D:
            return Diligent::RESOURCE_DIM_TEX_1D;
        case ResourceDimension::DIM_2D:
            return Diligent::RESOURCE_DIM_TEX_2D;
        case ResourceDimension::DIM_3D:
            return Diligent::RESOURCE_DIM_TEX_3D;
        case ResourceDimension::DIM_CUBE:
            return Diligent::RESOURCE_DIM_TEX_CUBE;
    }
    NC_ASSERT( false, "Unhandled ResourceDimension" );
    return Diligent::RESOURCE_DIM_UNDEFINED;
}

void DiligentTypeHelpers::apply_depth_stencil_op( Diligent::StencilOpDesc& to, const StencilOpDesc& from )
{
    Diligent::StencilOpDesc result{};
    result.StencilDepthFailOp = translate_stencil_op( from.depth_fail );
    result.StencilFailOp      = translate_stencil_op( from.fail );
    result.StencilPassOp      = translate_stencil_op( from.pass );
    result.StencilFunc        = translate_comp_func( from.func );
}

void DiligentTypeHelpers::apply_depth_stencil_state(
    Diligent::DepthStencilStateDesc& to, const DepthStencilStateDesc& from
)
{
    apply_depth_stencil_op( to.BackFace, from.back );
    apply_depth_stencil_op( to.FrontFace, from.front );
}

Diligent::FILL_MODE DiligentTypeHelpers::translate_fill_mode( FillMode mode )
{
    switch (mode) {
        case FillMode::SOLID:
            return Diligent::FILL_MODE_SOLID;
        case FillMode::WIREFRAME:
            return Diligent::FILL_MODE_WIREFRAME;
    }
    NC_ASSERT( false, "Unhandled FillMode" );
    return Diligent::FILL_MODE_UNDEFINED;
}

Diligent::VALUE_TYPE DiligentTypeHelpers::translate_value_type( ShaderValueType type )
{
    switch (type) {
        case ShaderValueType::FLOAT:
        case ShaderValueType::FLOAT2:
        case ShaderValueType::FLOAT3:
        case ShaderValueType::FLOAT4:
            return Diligent::VT_FLOAT32;
        case ShaderValueType::INT:
        case ShaderValueType::INT2:
        case ShaderValueType::INT3:
        case ShaderValueType::INT4:
            return Diligent::VT_INT32;
        case ShaderValueType::UBYTE4_NORM:
            return Diligent::VT_UINT8;
        case ShaderValueType::USHORT4:
            return Diligent::VT_UINT16;
        case ShaderValueType::BOOL:
            return Diligent::VT_INT32;
        case ShaderValueType::MAT4:
            break;
        case ShaderValueType::TEXTURE2D:
        case ShaderValueType::SAMPLER:
        case ShaderValueType::UNKNOWN:
            break;
    }
    NC_ASSERT( false, "Unhandled ValueType" );
    return Diligent::VT_UNDEFINED;
}

uint32_t DiligentTypeHelpers::translate_value_num_components( ShaderValueType type )
{
    switch (type) {
        case ShaderValueType::FLOAT:
            return 1;
        case ShaderValueType::FLOAT2:
            return 2;
        case ShaderValueType::FLOAT3:
            return 3;
        case ShaderValueType::FLOAT4:
            return 4;
        case ShaderValueType::INT:
            return 1;
        case ShaderValueType::INT2:
            return 2;
        case ShaderValueType::INT3:
            return 3;
        case ShaderValueType::INT4:
            return 4;
        case ShaderValueType::UBYTE4_NORM:
            return 4;
        case ShaderValueType::USHORT4:
            return 4;
        case ShaderValueType::BOOL:
            return 1;
        case ShaderValueType::MAT4:
            break;
        case ShaderValueType::TEXTURE2D:
        case ShaderValueType::SAMPLER:
        case ShaderValueType::UNKNOWN:
            break;
    }
    NC_ASSERT( false, "Unhandled ValueType" );
    return 0;
}

Diligent::SHADER_TYPE DiligentTypeHelpers::translate_shader_stage( ShaderType stage )
{
    switch (stage) {
        case ShaderType::VERTEX:
            return Diligent::SHADER_TYPE_VERTEX;
        case ShaderType::PIXEL:
            return Diligent::SHADER_TYPE_PIXEL;
        case ShaderType::MULTIPLE:
            return Diligent::SHADER_TYPE_VS_PS;
    }
    NC_ASSERT( false, "Unhandled ShaderType" );
    return Diligent::SHADER_TYPE_UNKNOWN;
}

Diligent::SHADER_RESOURCE_TYPE DiligentTypeHelpers::translate_resource_type( ResourceType type )
{
    switch (type) {
        case ResourceType::CONSTANT_BUFFER:
            return Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;
        case ResourceType::TEXTURE_SRV:
            return Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV;
        case ResourceType::BUFFER_SRV:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV;
        case ResourceType::TEXTURE_UAV:
            return Diligent::SHADER_RESOURCE_TYPE_TEXTURE_UAV;
        case ResourceType::BUFFER_UAV:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_UAV;
        case ResourceType::SAMPLER:
            return Diligent::SHADER_RESOURCE_TYPE_SAMPLER;
        case ResourceType::VARYING_INPUT:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV;
    }
    NC_ASSERT( false, "Unhandled ResourceType" );
    return Diligent::SHADER_RESOURCE_TYPE_UNKNOWN;
}

Diligent::SHADER_RESOURCE_VARIABLE_TYPE DiligentTypeHelpers::translate_shader_resource_var_type( ResourceVarType type )
{
    switch (type) {
        case ResourceVarType::STATIC:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        case ResourceVarType::MUTABLE:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        case ResourceVarType::DYNAMIC:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
    }
    NC_ASSERT( false, "Unhandled ResourceVarType" );
    return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
}

Diligent::PIPELINE_RESOURCE_FLAGS DiligentTypeHelpers::translate_pipeline_resource_flags( ResourceFlags flags )
{
    switch (flags) {
        case ResourceFlags::NONE:
            return Diligent::PIPELINE_RESOURCE_FLAG_NONE;
        case ResourceFlags::NO_DYNAMIC_BUFFERS:
            return Diligent::PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS;
        case ResourceFlags::COMBINED_SAMPLER:
            return Diligent::PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;
        case ResourceFlags::FORMATTED_BUFFER:
            return Diligent::PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER;
    }
    NC_ASSERT( false, "Unhandled ResourceFlags" );
    return Diligent::PIPELINE_RESOURCE_FLAG_NONE;
}

Diligent::PipelineResourceDesc DiligentTypeHelpers::translate_resource_desc( const PipelineResourceDesc& from )
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

Diligent::INPUT_ELEMENT_FREQUENCY DiligentTypeHelpers::translate_vertex_frequency( VertexFrequency freq )
{
    switch (freq) {
        case VertexFrequency::PER_VERTEX:
            return Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
        case VertexFrequency::PER_INSTANCE:
            return Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;
    }
    NC_ASSERT( false, "Unhandled VertexFrequency" );
    return Diligent::INPUT_ELEMENT_FREQUENCY_UNDEFINED;
}

} // namespace nc
