module types;

public struct SpatialGeometry {
    public float3 WorldPos;
    public float3 Normal;
    public float2 UV;
    public float2 UV2;
    public float4 Color;
    // Generic per-vertex varying interpolated to the fragment stage. A material's
    // vertex() can write here and its fragment()/prepare() can read it back.
    public float4 Custom;
    // Fragment screen-space UV in [0,1], filled in the pixel stage (not the vertex stage).
    public float2 ScreenUV;
}

public struct SurfaceOutput
{
    public float3 Albedo;
    public float3 Emission;
    public float Metallic;
    public float Roughness;
    public float Alpha;
    public SpatialGeometry Vertex;
};

public struct CanvasGeometry {
    public float2 WorldPos;
    public float2 UV;
    public float4 Color;
}
