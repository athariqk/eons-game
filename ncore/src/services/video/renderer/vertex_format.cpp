#include <ncore/services/video/renderer/vertex_format.h>

namespace nc {

gfx::VertexLayout get_vertex2d_layout()
{
    return {
        gfx::VertexLayoutElement{
            .location        = 0,
            .type            = gfx::ParameterType::FLOAT2,
            .stride          = 20,
            .relative_offset = 0,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "SV_Position",
        },
        gfx::VertexLayoutElement{
            .location        = 1,
            .type            = gfx::ParameterType::FLOAT2,
            .stride          = 20,
            .relative_offset = 8,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD1",
        },
        gfx::VertexLayoutElement{
            .location        = 2,
            .type            = gfx::ParameterType::UBYTE4_NORM,
            .stride          = 20,
            .relative_offset = 16,
            .normalized      = true,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD2",
        },
    };
}

gfx::VertexLayout get_vertex3d_layout()
{
    return {
        gfx::VertexLayoutElement{
            .location        = 0,
            .type            = gfx::ParameterType::FLOAT3,
            .stride          = 60,
            .relative_offset = 0,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "POSITION",
        },
        gfx::VertexLayoutElement{
            .location        = 1,
            .type            = gfx::ParameterType::FLOAT3,
            .stride          = 60,
            .relative_offset = 12,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "NORMAL",
        },
        gfx::VertexLayoutElement{
            .location        = 2,
            .type            = gfx::ParameterType::FLOAT4,
            .stride          = 60,
            .relative_offset = 24,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TANGENT",
        },
        gfx::VertexLayoutElement{
            .location        = 3,
            .type            = gfx::ParameterType::FLOAT2,
            .stride          = 60,
            .relative_offset = 40,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD0",
        },
        gfx::VertexLayoutElement{
            .location        = 4,
            .type            = gfx::ParameterType::FLOAT2,
            .stride          = 60,
            .relative_offset = 48,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD1",
        },
        gfx::VertexLayoutElement{
            .location        = 5,
            .type            = gfx::ParameterType::UBYTE4_NORM,
            .stride          = 60,
            .relative_offset = 56,
            .normalized      = true,
            .frequency       = gfx::VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "COLOR",
        },
    };
}

template<>
gfx::VertexLayout get_vertex_layout_for<Vertex2D>()
{
    return get_vertex2d_layout();
}
template<>
gfx::VertexLayout get_vertex_layout_for<Vertex3D>()
{
    return get_vertex3d_layout();
}

gfx::VertexLayout get_vertex_layout_by_name( StringView name )
{
    if (name == "Vertex2D")
        return get_vertex2d_layout();
    if (name == "Vertex3D")
        return get_vertex3d_layout();
    return {};
}

} // namespace nc
