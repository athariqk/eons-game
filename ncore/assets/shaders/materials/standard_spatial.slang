#include "spatial.slang"

import attributes;

[CullMode(2), DepthTest(1), DepthWrite(1), Blend(1)]
public struct StandardSpatial : ISpatialMaterial {
    Texture2D albedoTexture;
    SamplerState albedoSampler;
	uniform float4 albedoColor;
    uniform float roughness;
    uniform float metallic;

    void vertex(inout SpatialGeometry g) {
	}

    SurfaceOutput fragment(in SpatialGeometry g) {
        SurfaceOutput o;
        o.Albedo = albedoTexture.Sample(albedoSampler, g.UV).rgb * albedoColor.rgb;
        o.Alpha = albedoColor.a;
        o.Normal = g.Normal;
        o.Emission = 0;
        o.Metallic = metallic;
        o.Roughness = roughness;

        return o;
	}
};

ENTRY_POINTS(StandardSpatial);
