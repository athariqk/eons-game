#pragma once

#include <BlendState.h>
#include <DepthStencilState.h>
#include <GraphicsTypes.h>
#include <InputLayout.h>
#include <PipelineState.h>
#include <RasterizerState.h>

#include <ncore/services/video/gfx_types.h>

namespace nc {

struct DiligentTypeHelpers {
    static Diligent::TEXTURE_FORMAT translate_tex_format( gfx::TextureFormat format );
    static Diligent::PRIMITIVE_TOPOLOGY translate_prim_topology( gfx::PrimitiveTopology topology );
    static Diligent::CULL_MODE translate_cull( gfx::CullMode c );
    static Diligent::FILL_MODE translate_fill_mode( gfx::FillMode mode );
    static Diligent::COMPARISON_FUNCTION translate_comp_func( gfx::CompareFunc c );
    static Diligent::STENCIL_OP translate_stencil_op( gfx::StencilOp op );
    static Diligent::BLEND_OPERATION translate_blend_op( gfx::BlendOp op );
    static Diligent::BLEND_FACTOR translate_blend_factor( gfx::BlendFactor factor );
    static Diligent::RESOURCE_DIMENSION translate_resource_dim( gfx::ResourceDimension dim, uint32_t count = 1 );
    static Diligent::VALUE_TYPE translate_value_type( gfx::ParameterType type );
    static uint32_t translate_value_num_components( gfx::ParameterType type );
    static Diligent::SHADER_TYPE translate_shader_stage( gfx::ShaderStage stage );
    static Diligent::INPUT_ELEMENT_FREQUENCY translate_vertex_frequency( gfx::VertexFrequency freq );
    static Diligent::SHADER_RESOURCE_TYPE translate_resource_type( gfx::ResourceType type );
    static Diligent::SHADER_RESOURCE_VARIABLE_TYPE translate_shader_resource_var_type( gfx::ResourceBindType type );
    static Diligent::PIPELINE_RESOURCE_FLAGS translate_pipeline_resource_flags( gfx::ResourceFlags flags );
    static Diligent::PipelineResourceDesc translate_resource_desc( const gfx::PipelineResourceDesc& from );

    static void apply_depth_stencil_op( Diligent::StencilOpDesc& to, const gfx::StencilOpDesc& from );
    static void
    apply_depth_stencil_state( Diligent::DepthStencilStateDesc& to, const gfx::DepthStencilStateDesc& from );
};

} // namespace nc
