#include "spatial.slang"

import attributes;

// Port of 2Retr0/GodotOceanWaves assets/shaders/spatial/water.gdshader
// Lighting model ~ GDC 2019 "Interactive Water Simulation in Atlas" (GPUOpen).
// https://github.com/2Retr0/GodotOceanWaves/blob/main/assets/shaders/spatial/water.gdshader

static const uint NUM_CASCADES = 2;
static const float REFLECTANCE = 0.02; // air→water, eta≈1.33
static const float PI = 3.14159265;

float4 cubic_weights(float a)
{
    float a2 = a * a;
    float a3 = a2 * a;
    float w0 = -a3 + a2 * 3.0 - a * 3.0 + 1.0;
    float w1 = a3 * 3.0 - a2 * 6.0 + 4.0;
    float w2 = -a3 * 3.0 + a2 * 3.0 + a * 3.0 + 1.0;
    float w3 = a3;
    return float4(w0, w1, w2, w3) / 6.0;
}

// GPU Gems 2 ch.20 — bicubic B-spline on a Texture2DArray layer.
float4 texture_bicubic(Texture2DArray<float4> tex, SamplerState samp, float3 uvw)
{
    uint w = 0, h = 0, layers = 0, levels = 0;
    tex.GetDimensions(0, w, h, layers, levels);
    float2 dims = float2(float(w), float(h));
    float2 dims_inv = 1.0 / dims;

    float3 u = uvw;
    u.xy = u.xy * dims + 0.5;

    float2 fuv = frac(u.xy);
    float4 wx = cubic_weights(fuv.x);
    float4 wy = cubic_weights(fuv.y);

    float4 g = float4(wx.xz + wx.yw, wy.xz + wy.yw);
    float4 hgt = (float4(wx.yw, wy.yw) / g + float2(-1.5, 0.5).xyxy + floor(u.xy).xxyy) * dims_inv.xxyy;
    float2 wt = g.xz / (g.xz + g.yw);

    return lerp(
        lerp(tex.Sample(samp, float3(hgt.yw, u.z)), tex.Sample(samp, float3(hgt.xw, u.z)), wt.x),
        lerp(tex.Sample(samp, float3(hgt.yz, u.z)), tex.Sample(samp, float3(hgt.xz, u.z)), wt.x),
        wt.y
    );
}

// GodotOceanWaves call order (parameter names in source are inverted vs usage).
float smith_masking_shadowing(float cos_theta, float alpha)
{
    float a = cos_theta / (alpha * sqrt(max(1.0 - cos_theta * cos_theta, 1e-8)));
    float a_sq = a * a;
    return a < 1.6 ? (1.0 - 1.259 * a + 0.396 * a_sq) / (3.535 * a + 2.181 * a_sq) : 0.0;
}

float ggx_distribution(float cos_theta, float alpha)
{
    float a_sq = alpha * alpha;
    float d = 1.0 + (a_sq - 1.0) * cos_theta * cos_theta;
    return a_sq / (PI * d * d);
}

[CullMode(2), DepthTest(1), DepthWrite(1), Blend(1)]
public struct WaterMaterial : ISpatialMaterial {
    Texture2DArray<float4> displacementMap;
    Texture2DArray<float4> normalMap;
    TextureCube envMap;
    SamplerState sampler;

    // Per cascade: xy = 1/tile_length, z = displacement_scale, w = normal_scale
    uniform float4 mapScales[2];

    uniform float3 waterColor;
    uniform float3 foamColor;
    uniform float roughness;       // base roughness (~0.4 in Godot)
    uniform float normalStrength;  // global normal strength
    uniform float reflectionStrength;

    void vertex(inout SpatialGeometry g)
    {
        float2 uv = g.WorldPos.xz;
        float dist_cam = length(uv - CAMERA_POSITION.xz);
        // Displacement falls off after ~150 m (GodotOceanWaves).
        float distance_factor = min(exp(-(dist_cam - 150.0) * 0.007), 1.0);

        float3 displacement = 0;
        for (uint i = 0; i < NUM_CASCADES; ++i) {
            float4 scales = mapScales[i];
            displacement += displacementMap.SampleLevel(
                sampler, float3(uv * scales.xy, float(i)), 0.0).xyz * scales.z;
        }
        g.WorldPos += displacement * distance_factor;
        g.UV = uv;
        g.WaveHeight = displacement.y;

        // Cheap VS normal seed (fragment recomputes with bicubic).
        float2 grad = 0;
        for (uint i = 0; i < NUM_CASCADES; ++i) {
            float4 scales = mapScales[i];
            float4 nrm = normalMap.SampleLevel(sampler, float3(uv * scales.xy, float(i)), 0.0);
            grad += nrm.xy * scales.w;
        }
        g.Normal = normalize(float3(-grad.x, 1.0, -grad.y));
    }

    SurfaceOutput fragment(SpatialGeometry g)
    {
        float2 uv = g.UV;
        float dist = length(g.WorldPos.xz);

        uint mw = 0, mh = 0, mlayers = 0, mlevels = 0;
        normalMap.GetDimensions(0, mw, mh, mlayers, mlevels);
        float map_size = float(mw);

        // gradient.xy = slope, gradient.z accumulates foam (normal_map.a)
        float3 gradient = 0;
        for (uint i = 0; i < NUM_CASCADES; ++i) {
            float4 scales = mapScales[i];
            float3 coords = float3(uv * scales.xy, float(i));
            float ppm = map_size * min(scales.x, scales.y);
            float4 n_bic = texture_bicubic(normalMap, sampler, coords);
            float4 n_bil = normalMap.Sample(sampler, coords);
            float4 n_mix = lerp(n_bic, n_bil, min(1.0, ppm * 0.1));
            // Godot: .xyw * vec3(scales.ww, 1) → xy scaled by normal_scale, foam unscaled
            gradient += float3(n_mix.xy * scales.w, n_mix.a);
        }

        float foam_factor = smoothstep(0.0, 1.0, gradient.z * 0.75) * exp(-dist * 0.0075);
        float3 albedo = lerp(waterColor, foamColor, foam_factor);

        gradient.xy *= lerp(0.015, normalStrength, exp(-dist * 0.0175));
        float3 N = normalize(float3(-gradient.x, 1.0, -gradient.y));

        float3 V = normalize(CAMERA_POSITION - g.WorldPos);
        if (dot(N, V) < 0.0)
            N = -N;

        float alpha_rough = max(roughness, 1e-3);
        float fresnel = lerp(
            pow(1.0 - saturate(dot(V, N)), 5.0 * exp(-2.69 * alpha_rough))
                / (1.0 + 22.7 * pow(alpha_rough, 1.5)),
            1.0,
            REFLECTANCE
        );

        float surf_roughness = (1.0 - fresnel) * foam_factor + 0.4;

        // --- Lights (Godot light() port, world-space) ---
        float3 diffuse_acc = 0;
        float3 specular_acc = 0;
        const float3 sss_modifier = float3(0.9, 1.15, 0.85);
        float wave_height = g.WaveHeight;

        int light_count = g_LightEnv.count;
        for (int li = 0; li < light_count; ++li) {
            float3 L = normalize(g_LightEnv.lights[li].direction);
            float3 light_color = g_LightEnv.lights[li].intensity;
            float attenuation = 1.0;

            float3 halfway = normalize(L + V);
            float dot_nl = max(dot(N, L), 2e-5);
            float dot_nv = max(dot(N, V), 2e-5);

            // Match Godot call order: (roughness, cos) despite parameter names.
            float light_mask = smith_masking_shadowing(alpha_rough, dot_nv);
            float view_mask = smith_masking_shadowing(alpha_rough, dot_nl);
            float microfacet_distribution = ggx_distribution(dot(N, halfway), alpha_rough);
            float geometric_attenuation = 1.0 / (1.0 + light_mask + view_mask);

            specular_acc += fresnel * microfacet_distribution * geometric_attenuation
                            / (4.0 * dot_nv + 0.1) * attenuation * light_color;

            float sss_height = max(0.0, wave_height + 2.5)
                * pow(max(dot(L, -V), 0.0), 4.0)
                * pow(0.5 - 0.5 * dot(L, N), 3.0);
            float sss_near = 0.5 * pow(dot_nv, 2.0);
            float lambertian = 0.5 * dot_nl;

            float3 diffuse_term = lerp(
                (sss_height + sss_near) * sss_modifier / (1.0 + light_mask) + lambertian,
                foamColor,
                foam_factor
            );
            diffuse_acc += diffuse_term * (1.0 - fresnel) * attenuation * light_color;
        }

        // Environment reflection (Godot relies more on engine reflection probes;
        // we approximate with the sky cubemap).
        float3 R = reflect(-V, N);
        float3 env = envMap.Sample(sampler, R).rgb;
        float3 env_spec = env * fresnel * reflectionStrength;

        // Full lighting in Emission; Albedo left 0 so spatialPixelMain skips Lambert.
        SurfaceOutput o;
        o.Albedo = float3(0.0, 0.0, 0.0);
        o.Alpha = 1.0;
        o.Normal = N;
        o.Emission = albedo * diffuse_acc + specular_acc + env_spec;
        o.Metallic = 0.0;
        o.Roughness = surf_roughness;
        return o;
    }
};

ENTRY_POINTS(WaterMaterial);
