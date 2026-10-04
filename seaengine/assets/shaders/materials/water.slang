#include "spatial.slang"

import attributes;

// FFT normal_map layout (fft_unpack.slang): float4(dhy_dx, dhy_dz, dhx_dx, foam)
// gradient.xy builds a world-space normal with Y-up.

[CullMode(2), DepthTest(1), DepthWrite(1), Blend(1)]
public struct WaterMaterial : ISpatialMaterial {
    Texture2DArray<float4> displacementMap;
    Texture2DArray<float4> normalMap;
    TextureCube envMap;
    SamplerState sampler;

    // Per cascade: xy = 1/tile_length, z = displacement_scale, w = normal_scale
    uniform float4 mapScales[2];

    // Deep / shallow-ish base colors (linear-ish working space)
    uniform float3 deepColor;
    uniform float3 scatterColor;
    uniform float fresnelBias;   // F0-ish, ~0.02 for water
    uniform float reflectionStrength;
    uniform float foamStrength;

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

        // Seed a reasonable normal for rasterization / early path; fragment refines.
        float2 grad = 0;
        for (uint i = 0; i < 2; ++i) {
            float4 scales = mapScales[i];
            float4 nrm = normalMap.SampleLevel(
                sampler, float3(uv * scales.xy, float(i)), 0.0);
            grad += nrm.xy * scales.w;
        }
        g.Normal = normalize(float3(-grad.x, 1.0, -grad.y));
    }

    SurfaceOutput fragment(SpatialGeometry g) {
        float2 uv = g.UV;

        float2 grad = 0;
        float foam = 0;
        for (uint i = 0; i < 2; ++i) {
            float4 scales = mapScales[i];
            float4 nrm = normalMap.Sample(sampler, float3(uv * scales.xy, float(i)));
            grad += nrm.xy * scales.w;
            foam += nrm.a * scales.w;
        }
        foam = saturate(foam * foamStrength);

        float3 N = normalize(float3(-grad.x, 1.0, -grad.y));
        // Face the camera if the mesh is viewed from below
        float3 V = normalize(CAMERA_POSITION - g.WorldPos);
        if (dot(N, V) < 0.0)
            N = -N;

        float NdV = saturate(dot(N, V));
        float F0 = fresnelBias;
        float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdV, 5.0);

        float3 R = reflect(-V, N);
        float3 env = envMap.Sample(sampler, R).rgb;

        // Cheap height/scatter cue from displacement residual is not available here;
        // blend deep vs scatter by viewing angle (more scatter near horizon).
        float facing = NdV;
        float3 waterAlbedo = lerp(scatterColor, deepColor, facing);
        float3 foamColor = float3(0.92, 0.95, 0.98);
        float3 albedo = lerp(waterAlbedo, foamColor, foam);

        SurfaceOutput o;
        o.Albedo = albedo;
        o.Alpha = 1.0;
        o.Normal = N;
        // Specular-ish reflection via emission until a specular BRDF exists
        o.Emission = env * fresnel * reflectionStrength * (1.0 - foam * 0.85);
        o.Metallic = 0.0;
        o.Roughness = lerp(0.08, 0.55, foam);
        return o;
    }
};

ENTRY_POINTS(WaterMaterial);
