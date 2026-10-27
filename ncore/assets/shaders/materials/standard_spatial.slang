#include "spatial.slang"

import attributes;

// https://github.com/shader-slang/slang/blob/master/examples/model-viewer/shaders.slang
struct BlinnPhong : IBRDF
{
    // Blinn-Phong needs diffuse and specular reflectances, plus
    // a specular exponent value (which relates to "roughness"
    // in more modern physically-based models).
    //
    float3 kd;
    float3 ks;
    float specularity;

    // Here we implement the one requirement of the `IBRDF` interface
    // for our concrete implementation, using a textbook definition
    // of Blinng-Phong shading.
    //
    // Note: our "BRDF" definition here folds the N-dot-L term into
    // the evlauation of the reflectance function in case there are
    // useful algebraic simplifications this enables.
    //
    float3 evaluate(float3 V, float3 L, float3 N)
    {
        float nDotL = saturate(dot(N, L));
        float3 H = normalize(L + V);
        float nDotH = saturate(dot(N, H));

        return kd*nDotL + ks*pow(nDotH, specularity);
    }
};

[CullMode(2), DepthTest(1), DepthWrite(1), Blend(0)]
public struct StandardSpatial : ISpatialMaterial {
    Texture2D albedoTexture;
    SamplerState albedoSampler;
    [Color] uniform float4 albedoColor;
    [Range(0.0, 1.0)] uniform float roughness;
    [Range(0.0, 1.0)] uniform float metallic;
    [Color] uniform float3 specularColor;
    [Range(1.0, 256.0)] uniform float specularity;

    void vertex(inout SpatialGeometry g) {}

    SurfaceOutput fragment(in SpatialGeometry g) {
        SurfaceOutput o;
        o.Albedo = albedoTexture.Sample(albedoSampler, g.UV).rgb * albedoColor.rgb * g.Color.rgb;
        o.Alpha = albedoColor.a;
        o.Emission = 0;
        o.Metallic = metallic;
        o.Roughness = roughness;
        o.Vertex = g;
        return o;
	}

    typedef BlinnPhong BRDF;

    BlinnPhong prepare(SurfaceOutput s) {
        BlinnPhong brdf;
        brdf.kd = s.Albedo;
        brdf.ks = specularColor;
        brdf.specularity = specularity;
        return brdf;
	}
};

ENTRY_POINTS(StandardSpatial);
