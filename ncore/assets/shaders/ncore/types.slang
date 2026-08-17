public struct ShaderConstants
{
    // The object's current transform. Is just an ident matrix during 2D pass.
    public float4x4 ModelMatrix;
    public float4x4 ModelMatrixInv;
    // The camera's current transform.
    public float4x4 CameraMatrix;
    // This is pre-multiplied with projection matrix in the CPU.
    // Set as ortho projection during 2D pass; perspective during 3D pass.
    public float4x4 ViewProjMatrix;
    // Elapsed time in seconds.
    public float Time;
    // Elapsed time between frames.
    public float DeltaTime;


}

public struct Vertex2D
{
    public float2 Position;
    public float2 UV : TEXCOORD1;
    public float4 Color : TEXCOORD2;
};

// 3D vertex format
public struct Vertex3D
{
    public float3 Position; // In local space.
    public float3 Normal;
    public float4 Tangent;
    public float2 UV1 : TEXCOORD1;
    public float2 UV2 : TEXCOORD2;
    public float4 Color : TEXCOORD3;
};

public struct VSOut
{
    public float4 Position : SV_Position; // In screen/clip space.
    public float3 Normal;
    public float3 TexCoord : TEXCOORD0;
    public float4 Color : TEXCOORD1;
    public float2 UV : TEXCOORD2;
};

public struct Light
{

}
