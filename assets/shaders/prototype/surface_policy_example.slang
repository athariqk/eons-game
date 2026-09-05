// Prototype: surface policy defaults live next to the shader as a soft hint.
// Engine parses `// nc_pipeline:` into SurfacePolicy (not GPU state by itself).
// Depth / RT formats still come from the render pass when building the PSO.

// nc_pipeline: cull=back, fill=solid, blend=opaque, alpha=opaque, double_sided=false

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
};

struct VertexOutput {
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
};

cbuffer FrameConstants {
    float4x4 ModelMatrix;
    float4x4 ViewProjMatrix;
    float    Time;
    float    DeltaTime;
};

[shader("vertex")]
VertexOutput vs_main(VertexInput i)
{
    VertexOutput o;
    float4 wp = mul(ModelMatrix, float4(i.Position, 1.0));
    o.Position = mul(ViewProjMatrix, wp);
    o.Normal   = i.Normal;
    o.UV       = i.UV;
    return o;
}

// Fragment owns the "look" defaults; pass may still force depth-only / no blend.
[shader("fragment")]
float4 ps_main(VertexOutput i) : SV_Target
{
    float3 n = normalize(i.Normal) * 0.5 + 0.5;
    return float4(n, 1.0);
}
