module common;

public struct ShaderConstants
{
    // Is just an ident matrix during 2D pass.
    public float4x4 ModelMatrix;
    // This is pre-multiplied with projection matrix in the CPU.
	// An ortho projection during 2D pass and perspective during 3D pass.
    public float4x4 ViewMatrix;
	// Elapsed time in seconds.
    public float Time;
	// Elapsed time between frames.
    public float DeltaTime;
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

// Source: www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
public uint pcg_hash(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Returns a random float in [0, 1] range.
public float random_float(uint seed)
{
    uint random = pcg_hash(seed);
    float s = 1.0f / 4294967295.0f;
    return float(random) * s;
}

// Returns a "randomized" direction in unit vector from a seed.
public float3 random_direction(uint seed)
{
    uint random1 = pcg_hash(seed);
    uint random2 = pcg_hash(random1);
    uint random3 = pcg_hash(random2);
    float x = random_float(random1);
    float y = random_float(random2);
    float z = random_float(random3);
    return float3(x, y, z);
}

// Useful read: https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/#using-normally-distributed-random-numbers
public float3 random_point_in_sphere(uint seed)
{
    float3 dir = random_direction(seed);
    float r = pow(random_float(seed), 0.333f);
    return dir * r;
}
