// Design inspiration taken from https://github.com/shader-slang/slang/blob/master/examples/model-viewer

#include "scene.slang"
#include "light.slang"

public struct UnlitBRDF : IBRDF {
    float3 evaluate(float3 V, float3 L, float3 N)
    {
        return float3(0.0, 0.0, 0.0);
	}
}

public interface ISpatialMaterial {
    void vertex(inout SpatialGeometry g);
    SurfaceOutput fragment(in SpatialGeometry g);

    associatedtype BRDF : IBRDF;
    BRDF prepare(SurfaceOutput s);
};

struct Vertex3D
{
    float3 Position; // in local space.
    float3 Normal;
    float4 Tangent;
    float2 UV1 : TEXCOORD1;
    float2 UV2 : TEXCOORD2;
    float4 Color : TEXCOORD3;
};

struct VertexOutput
{
    float4 Position : SV_Position; // in screen/clip space.
    float3 WorldPos;
    float3 Normal;
    float4 Color : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float4 Custom : TEXCOORD3; // material varyings (see SpatialGeometry::Custom).
};

// TODO: make this work with any kind of lighting environment
[[vk::binding(1, 0)]] ConstantBuffer<LightArray<DirectionalLight, 4>> g_LightEnv;

struct SpatialInstance
{
    float4x4 ModelMatrix; // the object's current transform.
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
    // normalize(0) is NaN, keep a zero vector so the pixel stage can substitute a
    // geometric normal instead of propagating NaN (which renders as black).
    float3 worldNormal = (dot(n, n) > 1e-12) ? normalize(n) : float3(0.0, 0.0, 0.0);

    SpatialGeometry g;
    g.WorldPos = worldPos.xyz;
    g.Normal = worldNormal;
    g.UV = assembledVertex.UV1;
    g.UV2 = assembledVertex.UV2;
    g.Color = assembledVertex.Color;
    g.Custom = float4(0.0, 0.0, 0.0, 0.0);
    g.ScreenUV = float2(0.0, 0.0);

    material.vertex(g);

    float4 projectedPos = mul(VIEW_PROJ_MATRIX, float4(g.WorldPos, 1.0));

    VertexOutput output;
    output.Position = projectedPos;
    output.WorldPos = g.WorldPos;
    output.Normal = g.Normal;
    output.UV = g.UV;
    output.Color = assembledVertex.Color;
    output.Custom = g.Custom;
    return output;
}

float4 spatialPixelMain<M : ISpatialMaterial>(VertexOutput v, ParameterBlock<M> material) : SV_Target
{
    SpatialGeometry g;
    g.WorldPos = v.WorldPos;
    g.Normal = normalize(v.Normal);
	g.UV = v.UV;
    g.UV2 = float2(0.0, 0.0); // fragment doesn't receive UV2, zero to avoid uninitialized reads.
    g.Color = v.Color;
    g.Custom = v.Custom;
    g.ScreenUV = v.Position.xy / float2(RENDER_WIDTH, RENDER_HEIGHT);

    SurfaceOutput o = material.fragment(g);

    let brdf = material.prepare(o);

    float3 V = normalize(CAMERA_POSITION - o.Vertex.WorldPos);

    float3 lit = g_LightEnv.illuminate(o, brdf, V);
    lit += o.Emission;

    // emit premultiplied alpha.
    return float4(lit * o.Alpha, o.Alpha);
}

// Emits the concrete material block + entry points for a spatial material. Call once
// at the end of a material file with the material struct as the argument.
#define ENTRY_POINTS(M)																							   \
    [[vk::binding(0, 1)]] ParameterBlock<M> g_Material;                                                            \
    [shader("vertex")] VertexOutput vertexMain(Vertex3D v) { return spatialVertexMain<M>( v, g_Material ); }       \
    [shader("pixel")] float4 pixelMain(VertexOutput v) { return spatialPixelMain<M>( v, g_Material ); }
