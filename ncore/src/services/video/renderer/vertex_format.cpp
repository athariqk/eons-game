#include <ncore/services/video/renderer/vertex_format.h>

namespace nc {

rhi::VertexLayout get_vertex2d_layout()
{
    return {
        rhi::VertexLayoutElement{
            .location        = 0,
            .type            = rhi::ShaderValueType::FLOAT2,
            .stride          = 20,
            .relative_offset = 0,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "SV_Position",
        },
        rhi::VertexLayoutElement{
            .location        = 1,
            .type            = rhi::ShaderValueType::FLOAT2,
            .stride          = 20,
            .relative_offset = 8,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD1",
        },
        rhi::VertexLayoutElement{
            .location        = 2,
            .type            = rhi::ShaderValueType::UBYTE4_NORM,
            .stride          = 20,
            .relative_offset = 16,
            .normalized      = true,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD2",
        },
    };
}

rhi::VertexLayout get_vertex3d_layout()
{
    return {
        rhi::VertexLayoutElement{
            .location        = 0,
            .type            = rhi::ShaderValueType::FLOAT3,
            .stride          = 60,
            .relative_offset = 0,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "POSITION",
        },
        rhi::VertexLayoutElement{
            .location        = 1,
            .type            = rhi::ShaderValueType::FLOAT3,
            .stride          = 60,
            .relative_offset = 12,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "NORMAL",
        },
        rhi::VertexLayoutElement{
            .location        = 2,
            .type            = rhi::ShaderValueType::FLOAT4,
            .stride          = 60,
            .relative_offset = 24,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TANGENT",
        },
        rhi::VertexLayoutElement{
            .location        = 3,
            .type            = rhi::ShaderValueType::FLOAT2,
            .stride          = 60,
            .relative_offset = 40,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD0",
        },
        rhi::VertexLayoutElement{
            .location        = 4,
            .type            = rhi::ShaderValueType::FLOAT2,
            .stride          = 60,
            .relative_offset = 48,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD1",
        },
        rhi::VertexLayoutElement{
            .location        = 5,
            .type            = rhi::ShaderValueType::UBYTE4_NORM,
            .stride          = 60,
            .relative_offset = 56,
            .normalized      = true,
            .frequency       = rhi::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "COLOR",
        },
    };
}

template<>
rhi::VertexLayout get_vertex_layout_for<Vertex2D>()
{
    return get_vertex2d_layout();
}
template<>
rhi::VertexLayout get_vertex_layout_for<Vertex3D>()
{
    return get_vertex3d_layout();
}

rhi::VertexLayout get_vertex_layout_by_name( const std::string& name )
{
    if (name == "Vertex2D")
        return get_vertex2d_layout();
    if (name == "Vertex3D")
        return get_vertex3d_layout();
    return {};
}

} // namespace nc
