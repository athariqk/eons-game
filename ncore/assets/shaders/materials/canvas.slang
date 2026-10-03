// Design inspiration taken from https://github.com/shader-slang/slang/blob/master/examples/model-viewer

#include "common.slang"

public struct CanvasGeometry {
    public float2 WorldPos;
    public float2 UV;
    public float4 Color;
}

public interface ICanvasMaterial {
    void vertex(inout CanvasGeometry g);
    float4 fragment(in CanvasGeometry g);
};

struct Vertex2D
{
    float2 Position;
    float2 UV : TEXCOORD1;
    float4 Color : TEXCOORD2;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD1;
    float4 Color : TEXCOORD2;
};

struct CanvasInstance
{
    float4x4 ViewProjMatrix; // 2D ortho projection, updated once per canvas pass
};
[[vk::push_constant]] ConstantBuffer<CanvasInstance> g_Instance;

// 2D orthographic projection.
#define VIEW_PROJ_MATRIX g_Instance.ViewProjMatrix

VertexOutput canvasVertexMain<M : ICanvasMaterial>(Vertex2D assembledVertex, ParameterBlock<M> material)
{
    CanvasGeometry g;
    g.WorldPos = assembledVertex.Position;
    g.UV = assembledVertex.UV;
    g.Color = assembledVertex.Color;

    material.vertex(g);

    float4 screenPos = mul(VIEW_PROJ_MATRIX, float4(g.WorldPos, 0.0, 1.0));

    VertexOutput o;
    o.Position = screenPos;
    o.UV = g.UV;
    o.Color = g.Color;

    return o;
}

float4 canvasPixelMain<M : ICanvasMaterial>(VertexOutput coarseVertex, ParameterBlock<M> material) : SV_Target
{
    CanvasGeometry g;
    g.WorldPos = coarseVertex.Position.xy;
    g.UV = coarseVertex.UV;
    g.Color = coarseVertex.Color;

    return material.fragment(g);
}

// Emits the concrete material block + entry points for a canvas material. Call once
// at the end of a material file with the material struct as the argument.
#define ENTRY_POINTS(M)																								 \
    [[vk::binding(0, 1)]] ParameterBlock<M> g_Material;                                                              \
    [shader("vertex")] VertexOutput vertexMain(Vertex2D v) { return canvasVertexMain<M>( v, g_Material ); }          \
    [shader("pixel")] float4 pixelMain(VertexOutput v) { return canvasPixelMain<M>( v, g_Material ); }
