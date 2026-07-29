module common;

public struct ShaderConstants
{
    // Is just an ident matrix during 2D pass.
    public float4x4 ModelMatrix;

    // This is pre-multiplied with projection matrix in the CPU.
	// Ortho projection during 2D pass and perspective during 3D pass.
    public float4x4 ViewMatrix;
}

public struct Vertex2DIn
{
    public float2 Position;
    public float2 UV : TEXCOORD1;
    public float4 Color : TEXCOORD2;
};

public struct VertexOut
{
    public float4 Color : TEXCOORD0;
    public float2 UV : TEXCOORD1;
    public float4 Position : SV_Position;
};

public struct PixelIn
{
    public float4 Color : TEXCOORD0;
    public float2 UV : TEXCOORD1;
    public float4 Position : SV_Position;
};

public struct Vertex3DIn
{
    public float3 Position;
    public float3 Normals;
    public float4 Tangents;
    public float2 UV1 : TEXCOORD1;
    public float2 UV2 : TEXCOORD2;
    public float4 Color : TEXCOORD3;
};
