// Design inspiration taken from https://github.com/shader-slang/slang/blob/master/examples/model-viewer

#include "common.slang"
#include "light.slang"

public struct SurfaceOutput
{
    public float3 Albedo;
    public float3 Emission;
    public float Metallic;
    public float Roughness;
    public float Alpha;
    public float3 Normal; // world space
};

public interface ISpatialMaterial {
    void vertex(inout SpatialGeometry g);
    SurfaceOutput fragment(in SpatialGeometry g);
};

// 3D vertex format
struct Vertex3D
{
    float3 Position; // In local space.
    float3 Normal;
    float4 Tangent;
    float2 UV1 : TEXCOORD1;
    float2 UV2 : TEXCOORD2;
    float4 Color : TEXCOORD3;
};

struct VertexOutput
{
    float4 Position : SV_Position; // In screen/clip space.
    float3 WorldPos;
    float3 Normal;
    float4 Color : TEXCOORD1;
    float2 UV : TEXCOORD2;
};

struct LambertBRDF : IBRDF
{
    float3 albedo;

    float3 evaluate(float3 wo, float3 wi, float3 N)
    {
        float NdotL = max(dot(N, wi), 0.0);
        return albedo * NdotL;
    }
};

[[vk::binding(0, 0)]] ParameterBlock<LightArray<DirectionalLight, 4>> g_LightEnv;

struct SpatialInstance
{
    float4x4 ModelMatrix; // The object's current transform.
    float4x4 NormalMatrix;
};
[[vk::push_constant]] ConstantBuffer<SpatialInstance> g_Instance;

#define VIEW_PROJ_MATRIX g_Scene.ViewProjMatrix
#define MODEL_MATRIX g_Instance.ModelMatrix
#define NORMAL_MATRIX g_Instance.NormalMatrix

// Generic vertex/pixel templates. The material block is a type parameter so the
// concrete material struct (and its uniforms) is known at compile time after the
// material file instantiates these templates.
VertexOutput spatialVertexMain<M : ISpatialMaterial>(Vertex3D assembledVertex, ParameterBlock<M> material)
{
    float4 worldPos = mul(MODEL_MATRIX, float4(assembledVertex.Position, 1.0));
    float3 n = mul(float3x3(NORMAL_MATRIX), assembledVertex.Normal);
    // normalize(0) is NaN; keep a zero vector so the pixel stage can substitute a
    // geometric normal instead of propagating NaN (which renders as black).
    float3 worldNormal = (dot(n, n) > 1e-12) ? normalize(n) : float3(0.0, 0.0, 0.0);

    SpatialGeometry g;
    g.WorldPos = worldPos.xyz;
    g.Normal = worldNormal;
    g.UV = assembledVertex.UV1;

    material.vertex(g);

    float4 projectedPos = mul(VIEW_PROJ_MATRIX, float4(g.WorldPos, 1.0));

    VertexOutput output;
    output.Position = projectedPos;
    output.WorldPos = g.WorldPos;
    output.Normal = g.Normal;
    output.UV = g.UV;
    output.Color = assembledVertex.Color;
    return output;
}

float4 spatialPixelMain<M : ISpatialMaterial>(VertexOutput v, ParameterBlock<M> material) : SV_Target
{
    SpatialGeometry g;
    g.WorldPos = v.WorldPos;
    g.UV = v.UV;

    float3 V = normalize(CAMERA_POSITION - g.WorldPos);

    // Prefer the interpolated vertex normal. Only fall back to the geometric face
    // normal (screen-space derivatives) when the vertex normal is degenerate — most
    // meshes (including all current materials) supply real normals, so the fallback
    // is skipped entirely instead of being evaluated for every pixel.
    float3 N = v.Normal;
    float nLen2 = dot(N, N);
    if (nLen2 > 1e-12) {
        g.Normal = N * rsqrt(nLen2);
    } else {
        float3 geoNormal = cross(ddx(g.WorldPos), ddy(g.WorldPos));
        float geoLen2 = dot(geoNormal, geoNormal);
        geoNormal = (geoLen2 > 1e-12) ? geoNormal * rsqrt(geoLen2) : float3(0.0, 1.0, 0.0);
        if (dot(geoNormal, V) < 0.0)
            geoNormal = -geoNormal;
        g.Normal = geoNormal;
    }

    SurfaceOutput o = material.fragment(g);

    float3 diffuse = o.Albedo / 3.14159265;
    float3 lit = diffuse * g_LightEnv.illuminate(g, LambertBRDF(o.Albedo), V);
    lit += o.Emission;

    return float4(lit, o.Alpha);
}

// Emits the concrete material block + entry points for a spatial material. Call once
// at the end of a material file with the material struct as the argument.
#define ENTRY_POINTS(M)																							   \
    [[vk::binding(0, 1)]] ParameterBlock<M> g_Material;                                                            \
    [shader("vertex")] VertexOutput vertexMain(Vertex3D v) { return spatialVertexMain<M>( v, g_Material ); }       \
    [shader("pixel")] float4 pixelMain(VertexOutput v) { return spatialPixelMain<M>( v, g_Material ); }
