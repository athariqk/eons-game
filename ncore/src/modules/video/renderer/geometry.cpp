#include <ncore/modules/video/renderer/geometry.h>

namespace nc {

VertexLayout get_vertex2d_layout()
{
    return {
        VertexLayoutElement{
            .location        = 0,
            .type            = ShaderValueType::FLOAT2,
            .stride          = 20,
            .relative_offset = 0,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "SV_Position",
        },
        VertexLayoutElement{
            .location        = 1,
            .type            = ShaderValueType::FLOAT2,
            .stride          = 20,
            .relative_offset = 8,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD1",
        },
        VertexLayoutElement{
            .location        = 2,
            .type            = ShaderValueType::UBYTE4_NORM,
            .stride          = 20,
            .relative_offset = 16,
            .normalized      = true,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD2",
        },
    };
}

VertexLayout get_vertex3d_layout()
{
    return {
        VertexLayoutElement{
            .location        = 0,
            .type            = ShaderValueType::FLOAT3,
            .stride          = 52,
            .relative_offset = 0,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "POSITION",
        },
        VertexLayoutElement{
            .location        = 1,
            .type            = ShaderValueType::FLOAT3,
            .stride          = 52,
            .relative_offset = 12,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "NORMAL",
        },
        VertexLayoutElement{
            .location        = 2,
            .type            = ShaderValueType::FLOAT4,
            .stride          = 52,
            .relative_offset = 24,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TANGENT",
        },
        VertexLayoutElement{
            .location        = 3,
            .type            = ShaderValueType::FLOAT2,
            .stride          = 52,
            .relative_offset = 40,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD0",
        },
        VertexLayoutElement{
            .location        = 4,
            .type            = ShaderValueType::FLOAT2,
            .stride          = 52,
            .relative_offset = 48,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD1",
        },
        VertexLayoutElement{
            .location        = 5,
            .type            = ShaderValueType::UBYTE4_NORM,
            .stride          = 52,
            .relative_offset = 56,
            .normalized      = true,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "COLOR",
        },
    };
}

VertexLayout get_skinned_vertex3d_layout()
{
    return {
        VertexLayoutElement{
            .location        = 0,
            .type            = ShaderValueType::FLOAT3,
            .stride          = 76,
            .relative_offset = 0,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "POSITION",
        },
        VertexLayoutElement{
            .location        = 1,
            .type            = ShaderValueType::FLOAT3,
            .stride          = 76,
            .relative_offset = 12,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "NORMAL",
        },
        VertexLayoutElement{
            .location        = 2,
            .type            = ShaderValueType::FLOAT4,
            .stride          = 76,
            .relative_offset = 24,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TANGENT",
        },
        VertexLayoutElement{
            .location        = 3,
            .type            = ShaderValueType::FLOAT2,
            .stride          = 76,
            .relative_offset = 40,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD0",
        },
        VertexLayoutElement{
            .location        = 4,
            .type            = ShaderValueType::FLOAT2,
            .stride          = 76,
            .relative_offset = 48,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "TEXCOORD1",
        },
        VertexLayoutElement{
            .location        = 5,
            .type            = ShaderValueType::UBYTE4_NORM,
            .stride          = 76,
            .relative_offset = 56,
            .normalized      = true,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "COLOR",
        },
        VertexLayoutElement{
            .location        = 6,
            .type            = ShaderValueType::USHORT4,
            .stride          = 76,
            .relative_offset = 60,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "BLENDWEIGHTS",
        },
        VertexLayoutElement{
            .location        = 7,
            .type            = ShaderValueType::USHORT4,
            .stride          = 76,
            .relative_offset = 68,
            .frequency       = VertexFrequency::PER_VERTEX,
            .hlsl_semantic   = "BLENDINDICES",
        },
    };
}

template<>
VertexLayout get_vertex_layout_for<Vertex2D>()
{
    return get_vertex2d_layout();
}
template<>
VertexLayout get_vertex_layout_for<Vertex3D>()
{
    return get_vertex3d_layout();
}
template<>
VertexLayout get_vertex_layout_for<SkinnedVertex3D>()
{
    return get_skinned_vertex3d_layout();
}

VertexLayout get_vertex_layout_by_name( const std::string& name )
{
    if (name == "Vertex2D")
        return get_vertex2d_layout();
    if (name == "Vertex3D")
        return get_vertex3d_layout();
    if (name == "SkinnedVertex3D")
        return get_skinned_vertex3d_layout();
    return {};
}

} // namespace nc
