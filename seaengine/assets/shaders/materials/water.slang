#include "spatial.slang"

import attributes;

[CullMode(2), DepthTest(1), DepthWrite(1), Blend(1)]
public struct WaterMaterial : ISpatialMaterial {
    Texture2DArray<float4> displacementMap;
    Texture2DArray<float4> normalMap;
    SamplerState sampler;
    uniform float4 mapScales[2];

    void vertex(inout SpatialGeometry g) {
        float2 uv = g.WorldPos.xz;
        float3 displacement = 0;
        for (uint i = 0; i < 2; ++i) {
            float4 scales = mapScales[i];
            displacement += displacementMap.SampleLevel(
                sampler, float3(uv * scales.xy, float(i)), 0.0).xyz * scales.z;
        }
        g.WorldPos += displacement;
        g.UV = uv;
    }

    SurfaceOutput fragment(SpatialGeometry g) {
        SurfaceOutput o;
        o.Albedo = float3(1.0, 1.0, 1.0);
        o.Alpha = 1.0;
        o.Normal = g.Normal;
        o.Emission = 0.15;
        o.Metallic = 0.0;
        o.Roughness = 0.15;
        return o;
    }
};

ENTRY_POINTS(WaterMaterial);
