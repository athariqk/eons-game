// adapted from: https://github.com/2Retr0/GodotOceanWaves/blob/main/assets/shaders/spatial/water.gdshader

#include "spatial.slang"

import attributes;

#define MAX_CASCADES 8
static const float REFLECTANCE = 0.02;
static const float PI = 3.14159265359;

struct WaterBRDF : IBRDF {
    float3 albedo;
    float3 foamColor;
    float roughness; // alpha used in GGX / Smith
    float fresnel;
    float foam;
    float waveHeight;

    float smith_masking_shadowing(float cos_theta, float alpha) {
        float a = cos_theta / (alpha * sqrt(1.0 - cos_theta * cos_theta));
        float a_sq = a * a;
        return a < 1.6 ? (1.0 - 1.259 * a + 0.396 * a_sq) / (3.535 * a + 2.181 * a_sq) : 0.0;
    }

    float ggx_distribution(float cos_theta, float alpha) {
        float a_sq = alpha * alpha;
        float d = 1.0 + (a_sq - 1.0) * cos_theta * cos_theta;
        return a_sq / (PI * d * d);
    }

    float3 evaluate(float3 wo, float3 wi, float3 N) {
        float3 V = wo; // surface -> camera
        float3 L = wi; // surface -> light

        float3 halfway = normalize(L + V);
        float dot_nl = max(dot(N, L), 2e-5);
        float dot_nv = max(dot(N, V), 2e-5);

        // --- SPECULAR ---
        // smith_masking_shadowing(cos_theta, alpha): pass the direction cosine and
        // roughness. (The Godot original swaps these arguments; corrected here.)
        float light_mask = smith_masking_shadowing(dot_nl, roughness);
        float view_mask = smith_masking_shadowing(dot_nv, roughness);
        float D = ggx_distribution(dot(N, halfway), roughness);
        float G = 1.0 / (1.0 + light_mask + view_mask);

        float3 specular = fresnel * D * G / (4.0 * dot_nv + 0.1);

        // --- DIFFUSE ---
        const float3 sss_modifier = float3(0.9, 1.15, 0.85); // greener subsurface tint
        float sss_height = 1.0 * max(0.0, waveHeight + 2.5)
            * pow(max(dot(L, -V), 0.0), 4.0)
            * pow(0.5 - 0.5 * dot(L, N), 3.0);
        float sss_near = 0.5 * pow(dot_nv, 2.0);
        float lambertian = 0.5 * dot_nl;

        float3 diffuse = lerp(
            (sss_height + sss_near) * sss_modifier / (1.0 + light_mask) + lambertian,
            foamColor,
            foam
        ) * (1.0 - fresnel);

        // Fold the surface albedo into the shading result. The spatial pixel stage
        // feeds this BRDF's output straight through, so it must own the albedo.
        return diffuse * albedo + specular;
    }
};

[CullMode(0), DepthTest(1), DepthWrite(1), Blend(0)]
public struct WaterMaterial : ISpatialMaterial {
    Texture2DArray<float4> displacementMap;
    Texture2DArray<float4> normalMap;
    TextureCube envMap;
    SamplerState sampler;

    [Color] uniform float4 waterColor;
    [Color] uniform float4 foamColor;
    [Range(0.0, 1.0)] uniform float roughness;
    [Range(0.0, 2.0)] uniform float normalStrength;
    [Range(0.0, 2.0)] uniform float envStrength;
    [Range(0.0, 0.5)] uniform float3 absorption; // per-channel Beer-Lambert extinction (1/m)
    uniform float mapSize;                         // map resolution (uniform, avoids GetDimensions per pixel)
    uniform float horizonDistance;                 // distance at which water fades into the sky

    [Header("Waves")] uniform uint numCascades;
    [Header("Waves")] uniform float4 mapScales[MAX_CASCADES];
    [Header("Clipmap")] uniform float morphTransitionWidth; // seam morph band as a fraction of the level half-extent

	void vertex(inout SpatialGeometry g) {
        // Clipmap placement + seam morph (GPU Gems 2 ch. 2, CDLOD-style variant).
        float2 localXZ = g.UV;
        float2 levelInfo = g.UV2;
        float sp = levelInfo.x;
        float2 originL = round(CAMERA_POSITION.xz / sp) * sp;
        float2 morphed = localXZ;
        if (levelInfo.y > 0.0) {
            float2 ci = round(localXZ / sp);           // fine-lattice index
            float2 odd = ci - 2.0 * floor(ci * 0.5);   // 1 on odd cells (Euclidean mod, safe for negatives)
            float2 psp = 2.0 * sp;
            float2 delta = originL - round(CAMERA_POSITION.xz / psp) * psp;
            float cheb = max(abs(localXZ.x), abs(localXZ.y));
            float band = levelInfo.y * clamp(morphTransitionWidth, 0.05, 0.5);
            float alpha = saturate((cheb - (levelInfo.y - band)) / band * 1.001);
            morphed = localXZ - alpha * (odd * sp + delta);
        }

        g.WorldPos.xz = originL + morphed;
        float2 uv = g.WorldPos.xz;
        float3 displacement = 0;
        uint cascade_count = min(numCascades, (uint)MAX_CASCADES);
        for (uint i = 0; i < cascade_count; ++i) {
            float4 scales = mapScales[i];
            displacement += displacementMap.SampleLevel(sampler, float3(uv * scales.xy, float(i)), 0.0).xyz * scales.z;
        }
        g.WorldPos += displacement;
        // Pass the wave crest height to the fragment stage via the Custom varying.
        g.Custom.x = displacement.y;
        g.UV = uv;
    }

    float4 cubicWeights(float a) {
        float a2 = a * a;
        float a3 = a2 * a;
        float w0 = -a3 + a2 * 3.0 - a * 3.0 + 1.0;
        float w1 = a3 * 3.0 - a2 * 6.0 + 4.0;
        float w2 = -a3 * 3.0 + a2 * 3.0 + a * 3.0 + 1.0;
        float w3 = a3;
        return float4(w0, w1, w2, w3) / 6.0;
    }

    // Bicubic B-spline filtering (GPU Gems 2, ch. 20)
    float4 textureBicubic(Texture2DArray<float4> tex, SamplerState s, float3 uvw) {
        float2 dims = float2(mapSize, mapSize);
        float2 dimsInv = 1.0 / dims;

        uvw.xy = uvw.xy * dims + 0.5;
        float2 fuv = frac(uvw.xy);

        float4 wx = cubicWeights(fuv.x);
        float4 wy = cubicWeights(fuv.y);
        float4 g = float4(wx.xz + wx.yw, wy.xz + wy.yw);
        float4 h = (float4(wx.yw, wy.yw) / g + float2(-1.5, 0.5).xyxy + floor(uvw.xy).xxyy) * dimsInv.xxyy;
        float2 w = g.xz / (g.xz + g.yw);

        return lerp(
			lerp(tex.Sample(s, float3(h.yw, uvw.z)), tex.Sample(s, float3(h.xw, uvw.z)), w.x),
			lerp(tex.Sample(s, float3(h.yz, uvw.z)), tex.Sample(s, float3(h.xz, uvw.z)), w.x), w.y);
    }

    SurfaceOutput fragment(SpatialGeometry g) {
        // Distance from the camera (the water follows the camera, so this must not be
        // measured from the world origin).
        float dist = length((g.WorldPos - CAMERA_POSITION).xz);

        // Gradient (xy) and foam (z) accumulated over cascades
        float3 gradient = 0;
        uint cascade_count = min(numCascades, (uint)MAX_CASCADES);
        for (uint i = 0; i < cascade_count; ++i) {
            float4 scales = mapScales[i];
            float3 coords = float3(g.UV * scales.xy, float(i));
            float ppm = mapSize * min(scales.x, scales.y); // pixels per meter

            float3 grad = lerp(textureBicubic(normalMap, sampler, coords), normalMap.Sample(sampler, coords), min(1.0, ppm * 0.1)).xyw;
            gradient += grad * float3(scales.ww, 1.0);
        }

        gradient *= lerp(0.015, normalStrength, exp(-dist * 0.0005));
        float3 N = normalize(float3(-gradient.x, 1.0, -gradient.y));
        float3 V = normalize(CAMERA_POSITION - g.WorldPos);

        float foam_factor = smoothstep(0.0, 1.0, gradient.z * 0.75) * exp(-dist * 0.0005);
        float fresnel = lerp(pow(saturate(1.0 - dot(V, N)), 5.0 * exp(-2.69 * roughness)) / (1.0 + 22.7 * pow(roughness, 1.5)), 1.0, REFLECTANCE);

        SurfaceOutput o;
        o.Albedo = lerp(waterColor, foamColor, foam_factor).rgb;
        o.Alpha = 1.0;
        o.Metallic = 0.0;
        o.Roughness = (1.0 - fresnel) * foam_factor + 0.4; // Roughness is proportional to foam/fog amount and fresnel.
        o.Emission = envMap.Sample(sampler, reflect(-V, N)).rgb * fresnel * envStrength;

        float fog = smoothstep(0.0, max(horizonDistance, 1.0), dist);
        if (fog > 0.0) {
            float3 sky_col = envMap.Sample(sampler, normalize(g.WorldPos - CAMERA_POSITION)).rgb;
            o.Albedo *= (1.0 - fog);
            o.Emission = lerp(o.Emission, sky_col, fog);
        }

        o.Vertex = g;
        o.Vertex.Normal = N;
        o.Vertex.Custom.y = fresnel;
        o.Vertex.Custom.z = foam_factor;

        return o;
    }

    typedef WaterBRDF BRDF;

    WaterBRDF prepare(SurfaceOutput s) {
        WaterBRDF brdf;
        brdf.albedo = s.Albedo;
        brdf.foamColor = foamColor.rgb;
        brdf.roughness = s.Roughness;
        brdf.fresnel = s.Vertex.Custom.y;
        brdf.foam = s.Vertex.Custom.z;
        brdf.waveHeight = s.Vertex.Custom.x; // interpolated displacement.y from the vertex stage
        return brdf;
	}
};

ENTRY_POINTS(WaterMaterial);
