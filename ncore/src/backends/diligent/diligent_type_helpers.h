#pragma once

#include <BlendState.h>
#include <DepthStencilState.h>
#include <GraphicsTypes.h>
#include <InputLayout.h>
#include <PipelineState.h>
#include <RasterizerState.h>

#include <ncore/services/video/rhi_types.h>

namespace nc {

struct DiligentTypeHelpers {
    static Diligent::TEXTURE_FORMAT translate_tex_format( TextureFormat format );
    static Diligent::PRIMITIVE_TOPOLOGY translate_prim_topology( PrimitiveTopology topology );
    static Diligent::CULL_MODE translate_cull( CullMode c );
    static Diligent::FILL_MODE translate_fill_mode( FillMode mode );
    static Diligent::COMPARISON_FUNCTION translate_comp_func( CompareFunc c );
    static Diligent::STENCIL_OP translate_stencil_op( StencilOp op );
    static Diligent::BLEND_OPERATION translate_blend_op( BlendOp op );
    static Diligent::BLEND_FACTOR translate_blend_factor( BlendFactor factor );
    static Diligent::RESOURCE_DIMENSION translate_resource_dim( ResourceDimension dim );
    static Diligent::VALUE_TYPE translate_value_type( ShaderValueType type );
    static uint32_t translate_value_num_components( ShaderValueType type );
    static Diligent::SHADER_TYPE translate_shader_stage( ShaderType stage );
    static Diligent::INPUT_ELEMENT_FREQUENCY translate_vertex_frequency( VertexFrequency freq );
    static Diligent::SHADER_RESOURCE_TYPE translate_resource_type( ResourceType type );
    static Diligent::SHADER_RESOURCE_VARIABLE_TYPE translate_shader_resource_var_type( ResourceVarType type );
    static Diligent::PIPELINE_RESOURCE_FLAGS translate_pipeline_resource_flags( ResourceFlags flags );
    static Diligent::PipelineResourceDesc translate_resource_desc( const PipelineResourceDesc& from );

    static void apply_depth_stencil_op( Diligent::StencilOpDesc& to, const StencilOpDesc& from );
    static void apply_depth_stencil_state( Diligent::DepthStencilStateDesc& to, const DepthStencilStateDesc& from );
};

} // namespace nc
