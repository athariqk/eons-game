// Code inspiration taken from https://github.com/shader-slang/slang/blob/master/examples/model-viewer

public struct SpatialGeometry {
    public float3 WorldPos;
    public float3 Normal;
    public float2 UV;
    // Material-defined scalar (e.g. water wave height for SSS).
    public float WaveHeight;
}

public interface IBRDF
{
    public float3 evaluate(float3 wo, float3 wi, float3 N);
};

public interface ILightEnv
{
    public float3 illuminate<B : IBRDF>(SpatialGeometry g, B brdf, float3 wo);
};

public struct LightArray<L : ILightEnv, let N : int> : ILightEnv
{
    public int count;
    public L lights[N];

    public float3 illuminate<B : IBRDF>(SpatialGeometry g, B brdf, float3 wo)
    {
        float3 sum = 0;
        for (int ii = 0; ii < count; ++ii)
        {
            sum += lights[ii].illuminate(g, brdf, wo);
        }
        return sum;
    }
};

public struct LightPair<T : ILightEnv, U : ILightEnv> : ILightEnv
{
    public T first;
    public U second;

    public float3 illuminate<B : IBRDF>(SpatialGeometry g, B brdf, float3 wo)
    {
        return first.illuminate(g, brdf, wo)
            + second.illuminate(g, brdf, wo);
    }
};

// ---------------------------------------------------------------------------

struct DirectionalLight : ILightEnv
{
    float3 direction;
    float3 intensity;

    float3 illuminate<B : IBRDF>(SpatialGeometry g, B brdf, float3 wo)
    {
        return intensity * brdf.evaluate(wo, direction, g.Normal);
    }
};

struct PointLight : ILightEnv
{
    float3 position;
    float3 intensity;

    float3 illuminate<B : IBRDF>(SpatialGeometry g, B brdf, float3 wo)
    {
        float3 delta = position - g.WorldPos;
        float d = length(delta);
        float3 direction = normalize(delta);
        float3 illuminance = intensity / (d * d);
        return illuminance * brdf.evaluate(wo, direction, g.Normal);
    }
};
