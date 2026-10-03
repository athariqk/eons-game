#include "diligent_type_helpers.h"

namespace nc {

Diligent::TEXTURE_FORMAT DiligentTypeHelpers::translate_tex_format( gfx::TextureFormat format )
{
    switch (format) {
        case gfx::TextureFormat::D32_FLOAT:
            return Diligent::TEX_FORMAT_D32_FLOAT;
        case gfx::TextureFormat::RGBA8_UNORM:
            return Diligent::TEX_FORMAT_RGBA8_UNORM;
        case gfx::TextureFormat::RGBA8_UNORM_SRGB:
            return Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
        case gfx::TextureFormat::R32_FLOAT:
            return Diligent::TEX_FORMAT_R32_FLOAT;
        case gfx::TextureFormat::RG32_FLOAT:
            return Diligent::TEX_FORMAT_RG32_FLOAT;
        case gfx::TextureFormat::RGBA32_FLOAT:
            return Diligent::TEX_FORMAT_RGBA32_FLOAT;
        case gfx::TextureFormat::UNKNOWN:
            return Diligent::TEX_FORMAT_UNKNOWN;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::TextureFormat" );
    return Diligent::TEX_FORMAT_UNKNOWN;
}

Diligent::PRIMITIVE_TOPOLOGY DiligentTypeHelpers::translate_prim_topology( gfx::PrimitiveTopology topology )
{
    switch (topology) {
        case gfx::PrimitiveTopology::TRIANGLE_LIST:
            return Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case gfx::PrimitiveTopology::POINT_LIST:
            return Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::PrimitiveTopology" );
    return Diligent::PRIMITIVE_TOPOLOGY_UNDEFINED;
}

Diligent::CULL_MODE DiligentTypeHelpers::translate_cull( gfx::CullMode c )
{
    switch (c) {
        case gfx::CullMode::NONE:
            return Diligent::CULL_MODE_NONE;
        case gfx::CullMode::FRONT:
            return Diligent::CULL_MODE_FRONT;
        case gfx::CullMode::BACK:
            return Diligent::CULL_MODE_BACK;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::CullMode" );
    return Diligent::CULL_MODE_NONE;
}

Diligent::COMPARISON_FUNCTION DiligentTypeHelpers::translate_comp_func( gfx::CompareFunc c )
{
    switch (c) {
        case gfx::CompareFunc::NEVER:
            return Diligent::COMPARISON_FUNC_NEVER;
        case gfx::CompareFunc::LESS:
            return Diligent::COMPARISON_FUNC_LESS;
        case gfx::CompareFunc::EQUAL:
            return Diligent::COMPARISON_FUNC_EQUAL;
        case gfx::CompareFunc::LESS_EQUAL:
            return Diligent::COMPARISON_FUNC_LESS_EQUAL;
        case gfx::CompareFunc::GREATER:
            return Diligent::COMPARISON_FUNC_GREATER;
        case gfx::CompareFunc::NOT_EQUAL:
            return Diligent::COMPARISON_FUNC_NOT_EQUAL;
        case gfx::CompareFunc::GREATER_EQUAL:
            return Diligent::COMPARISON_FUNC_GREATER_EQUAL;
        case gfx::CompareFunc::ALWAYS:
            return Diligent::COMPARISON_FUNC_ALWAYS;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::CompareFunc" );
    return Diligent::COMPARISON_FUNC_UNKNOWN;
}

Diligent::STENCIL_OP DiligentTypeHelpers::translate_stencil_op( gfx::StencilOp op )
{
    switch (op) {
        case gfx::StencilOp::KEEP:
            return Diligent::STENCIL_OP_KEEP;
        case gfx::StencilOp::ZERO:
            return Diligent::STENCIL_OP_ZERO;
        case gfx::StencilOp::REPLACE:
            return Diligent::STENCIL_OP_REPLACE;
        case gfx::StencilOp::INCR_CLAMP:
            return Diligent::STENCIL_OP_INCR_SAT;
        case gfx::StencilOp::DECR_CLAMP:
            return Diligent::STENCIL_OP_DECR_SAT;
        case gfx::StencilOp::INVERT:
            return Diligent::STENCIL_OP_INVERT;
        case gfx::StencilOp::INCR_WRAP:
            return Diligent::STENCIL_OP_INCR_WRAP;
        case gfx::StencilOp::DECR_WRAP:
            return Diligent::STENCIL_OP_DECR_WRAP;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::StencilOp" );
    return Diligent::STENCIL_OP_UNDEFINED;
}

Diligent::BLEND_OPERATION DiligentTypeHelpers::translate_blend_op( gfx::BlendOp op )
{
    switch (op) {
        case gfx::BlendOp::ADD:
            return Diligent::BLEND_OPERATION_ADD;
        case gfx::BlendOp::SUBTRACT:
            return Diligent::BLEND_OPERATION_SUBTRACT;
        case gfx::BlendOp::REV_SUBTRACT:
            return Diligent::BLEND_OPERATION_REV_SUBTRACT;
        case gfx::BlendOp::MIN:
            return Diligent::BLEND_OPERATION_MIN;
        case gfx::BlendOp::MAX:
            return Diligent::BLEND_OPERATION_MAX;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::BlendOp" );
    return Diligent::BLEND_OPERATION_UNDEFINED;
}

Diligent::BLEND_FACTOR DiligentTypeHelpers::translate_blend_factor( gfx::BlendFactor factor )
{
    switch (factor) {
        case nc::gfx::BlendFactor::ZERO:
            return Diligent::BLEND_FACTOR_ZERO;
        case nc::gfx::BlendFactor::ONE:
            return Diligent::BLEND_FACTOR_ONE;
        case nc::gfx::BlendFactor::SRC_COLOR:
            return Diligent::BLEND_FACTOR_SRC_COLOR;
        case nc::gfx::BlendFactor::INV_SRC_COLOR:
            return Diligent::BLEND_FACTOR_INV_SRC_COLOR;
        case nc::gfx::BlendFactor::SRC_ALPHA:
            return Diligent::BLEND_FACTOR_SRC_ALPHA;
        case nc::gfx::BlendFactor::INV_SRC_ALPHA:
            return Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        case nc::gfx::BlendFactor::DST_COLOR:
            return Diligent::BLEND_FACTOR_DEST_COLOR;
        case nc::gfx::BlendFactor::INV_DST_COLOR:
            return Diligent::BLEND_FACTOR_INV_DEST_COLOR;
        case nc::gfx::BlendFactor::DST_ALPHA:
            return Diligent::BLEND_FACTOR_DEST_ALPHA;
        case nc::gfx::BlendFactor::INV_DST_ALPHA:
            return Diligent::BLEND_FACTOR_INV_DEST_ALPHA;
        case nc::gfx::BlendFactor::CONSTANT_COLOR:
            return Diligent::BLEND_FACTOR_BLEND_FACTOR;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::BlendFactor" );
    return Diligent::BLEND_FACTOR_UNDEFINED;
}

Diligent::RESOURCE_DIMENSION DiligentTypeHelpers::translate_resource_dim( gfx::ResourceDimension dim, uint32_t count )
{
    switch (dim) {
        case gfx::ResourceDimension::DIM_1D:
            return count > 1 ? Diligent::RESOURCE_DIM_TEX_1D_ARRAY : Diligent::RESOURCE_DIM_TEX_1D;
        case gfx::ResourceDimension::DIM_1D_ARRAY:
            return Diligent::RESOURCE_DIM_TEX_1D_ARRAY;
        case gfx::ResourceDimension::DIM_2D:
            return count > 1 ? Diligent::RESOURCE_DIM_TEX_2D_ARRAY : Diligent::RESOURCE_DIM_TEX_2D;
        case gfx::ResourceDimension::DIM_2D_ARRAY:
            return Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        case gfx::ResourceDimension::DIM_3D:
            return Diligent::RESOURCE_DIM_TEX_3D;
        case gfx::ResourceDimension::DIM_CUBE:
            return count > 6 ? Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY : Diligent::RESOURCE_DIM_TEX_CUBE;
        case gfx::ResourceDimension::DIM_CUBE_ARRAY:
            return Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::ResourceDimension" );
    return Diligent::RESOURCE_DIM_UNDEFINED;
}

void DiligentTypeHelpers::apply_depth_stencil_op( Diligent::StencilOpDesc& to, const gfx::StencilOpDesc& from )
{
    Diligent::StencilOpDesc result{};
    result.StencilDepthFailOp = translate_stencil_op( from.depth_fail );
    result.StencilFailOp      = translate_stencil_op( from.fail );
    result.StencilPassOp      = translate_stencil_op( from.pass );
    result.StencilFunc        = translate_comp_func( from.func );
}

void DiligentTypeHelpers::apply_depth_stencil_state(
    Diligent::DepthStencilStateDesc& to, const gfx::DepthStencilStateDesc& from
)
{
    apply_depth_stencil_op( to.BackFace, from.back );
    apply_depth_stencil_op( to.FrontFace, from.front );
}

Diligent::FILL_MODE DiligentTypeHelpers::translate_fill_mode( gfx::FillMode mode )
{
    switch (mode) {
        case gfx::FillMode::SOLID:
            return Diligent::FILL_MODE_SOLID;
        case gfx::FillMode::WIREFRAME:
            return Diligent::FILL_MODE_WIREFRAME;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::FillMode" );
    return Diligent::FILL_MODE_UNDEFINED;
}

Diligent::VALUE_TYPE DiligentTypeHelpers::translate_value_type( gfx::ParameterType type )
{
    switch (type) {
        // Floating Point Types
        case gfx::ParameterType::FLOAT_16:
            return Diligent::VT_FLOAT16;
        case gfx::ParameterType::FLOAT_32: // Also covers gfx::ParameterType::FLOAT
        case gfx::ParameterType::FLOAT2:
        case gfx::ParameterType::FLOAT3:
        case gfx::ParameterType::FLOAT4:
        case gfx::ParameterType::MAT4:
        case gfx::ParameterType::UFLOAT:
            return Diligent::VT_FLOAT32;
        case gfx::ParameterType::FLOAT_64:
            return Diligent::VT_FLOAT64;

        // Signed Integer Types
        case gfx::ParameterType::INT_8:
            return Diligent::VT_INT8;
        case gfx::ParameterType::INT_16:
            return Diligent::VT_INT16;
        case gfx::ParameterType::INT_32: // Also covers gfx::ParameterType::INT
        case gfx::ParameterType::INT2:
        case gfx::ParameterType::INT3:
        case gfx::ParameterType::INT4:
        case gfx::ParameterType::BOOL:
            return Diligent::VT_INT32;
        case gfx::ParameterType::INT_64:
            return Diligent::VT_UNDEFINED;

        // Unsigned Integer Types
        case gfx::ParameterType::UBYTE4_NORM:
            return Diligent::VT_UINT8;
        case gfx::ParameterType::USHORT:
        case gfx::ParameterType::USHORT4:
            return Diligent::VT_UINT16;
        case gfx::ParameterType::UINT:
            return Diligent::VT_UINT32;

        // Non-Value Types (Textures, Samplers, Pipeline Resources)
        case gfx::ParameterType::TEXTURE_2D:
        case gfx::ParameterType::TEXTURE_CUBED:
        case gfx::ParameterType::SAMPLER:
        case gfx::ParameterType::UNKNOWN:
            return Diligent::VT_UNDEFINED;
        default:
            break;
    }

    NC_ASSERT_MSG( false, "Unhandled gfx::ParameterType" );
    return Diligent::VT_UNDEFINED;
}
uint32_t DiligentTypeHelpers::translate_value_num_components( gfx::ParameterType type )
{
    switch (type) {
        // Single-component scalars
        case gfx::ParameterType::FLOAT_16:
        case gfx::ParameterType::FLOAT_32:
        case gfx::ParameterType::FLOAT_64:
        case gfx::ParameterType::INT_8:
        case gfx::ParameterType::INT_16:
        case gfx::ParameterType::INT_32:
        case gfx::ParameterType::INT_64:
        case gfx::ParameterType::UINT:
        case gfx::ParameterType::USHORT:
        case gfx::ParameterType::UFLOAT:
        case gfx::ParameterType::BOOL:
            return 1;

        // 2-component vectors
        case gfx::ParameterType::FLOAT2:
        case gfx::ParameterType::INT2:
            return 2;

        // 3-component vectors
        case gfx::ParameterType::FLOAT3:
        case gfx::ParameterType::INT3:
            return 3;

        // 4-component vectors / packed formats
        case gfx::ParameterType::FLOAT4:
        case gfx::ParameterType::INT4:
        case gfx::ParameterType::UBYTE4_NORM:
        case gfx::ParameterType::USHORT4:
            return 4;

        // 4x4 Matrices (16 total scalar components)
        case gfx::ParameterType::MAT4:
            return 16;

        // Non-value types (Textures, Samplers, Pipeline Resources)
        case gfx::ParameterType::TEXTURE_2D:
        case gfx::ParameterType::TEXTURE_CUBED:
        case gfx::ParameterType::SAMPLER:
        case gfx::ParameterType::UNKNOWN:
            return 0;
        default:
            break;
    }

    NC_ASSERT_MSG( false, "Unhandled gfx::ParameterType" );
    return 0;
}

Diligent::SHADER_TYPE DiligentTypeHelpers::translate_shader_stage( gfx::ShaderStage stage )
{
    switch (stage) {
        case gfx::ShaderStage::NONE:
            return Diligent::SHADER_TYPE_UNKNOWN;
        case gfx::ShaderStage::VERTEX:
            return Diligent::SHADER_TYPE_VERTEX;
        case gfx::ShaderStage::PIXEL:
            return Diligent::SHADER_TYPE_PIXEL;
        case gfx::ShaderStage::COMPUTE:
            return Diligent::SHADER_TYPE_COMPUTE;
        case gfx::ShaderStage::VS_PS:
            return Diligent::SHADER_TYPE_VS_PS;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::ShaderStage" );
    return Diligent::SHADER_TYPE_UNKNOWN;
}

Diligent::SHADER_RESOURCE_TYPE DiligentTypeHelpers::translate_resource_type( gfx::ResourceType type )
{
    switch (type) {
        case gfx::ResourceType::UNKNOWN:
            return Diligent::SHADER_RESOURCE_TYPE_UNKNOWN;
        case gfx::ResourceType::CONSTANT_BUFFER:
            return Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;
        case gfx::ResourceType::TEXTURE_SRV:
            return Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV;
        case gfx::ResourceType::BUFFER_SRV:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV;
        case gfx::ResourceType::TEXTURE_UAV:
            return Diligent::SHADER_RESOURCE_TYPE_TEXTURE_UAV;
        case gfx::ResourceType::BUFFER_UAV:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_UAV;
        case gfx::ResourceType::SAMPLER:
            return Diligent::SHADER_RESOURCE_TYPE_SAMPLER;
        case gfx::ResourceType::VARYING_INPUT:
            return Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::ResourceType" );
    return Diligent::SHADER_RESOURCE_TYPE_UNKNOWN;
}

Diligent::SHADER_RESOURCE_VARIABLE_TYPE
DiligentTypeHelpers::translate_shader_resource_var_type( gfx::ResourceBindType type )
{
    switch (type) {
        case gfx::ResourceBindType::STATIC:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        case gfx::ResourceBindType::MUTABLE:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        case gfx::ResourceBindType::DYNAMIC:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::ResourceBindType" );
    return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
}

Diligent::PIPELINE_RESOURCE_FLAGS DiligentTypeHelpers::translate_pipeline_resource_flags( gfx::ResourceFlags flags )
{
    switch (flags) {
        case gfx::ResourceFlags::NONE:
            return Diligent::PIPELINE_RESOURCE_FLAG_NONE;
        case gfx::ResourceFlags::NO_DYNAMIC_BUFFERS:
            return Diligent::PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS;
        case gfx::ResourceFlags::COMBINED_SAMPLER:
            return Diligent::PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;
        case gfx::ResourceFlags::FORMATTED_BUFFER:
            return Diligent::PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER;
        case gfx::ResourceFlags::INLINE_CONSTANTS:
            return Diligent::PIPELINE_RESOURCE_FLAG_INLINE_CONSTANTS;
        case gfx::ResourceFlags::RUNTIME_ARRAY:
            return Diligent::PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::ResourceFlags" );
    return Diligent::PIPELINE_RESOURCE_FLAG_NONE;
}

Diligent::PipelineResourceDesc DiligentTypeHelpers::translate_resource_desc( const gfx::PipelineResourceDesc& from )
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

Diligent::INPUT_ELEMENT_FREQUENCY DiligentTypeHelpers::translate_vertex_frequency( gfx::VertexFrequency freq )
{
    switch (freq) {
        case gfx::VertexFrequency::PER_VERTEX:
            return Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
        case gfx::VertexFrequency::PER_INSTANCE:
            return Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;
    }
    NC_ASSERT_MSG( false, "Unhandled gfx::VertexFrequency" );
    return Diligent::INPUT_ELEMENT_FREQUENCY_UNDEFINED;
}

} // namespace nc
