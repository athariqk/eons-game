#include "spatial.slang"

import attributes;

[CullMode(1), DepthTest(1), DepthWrite(1), Blend(0)]
public struct SkyboxMaterial : ISpatialMaterial {
    TextureCube cubemap;
    SamplerState sampler;

    void vertex(inout SpatialGeometry g) {
        // Infinite skybox: treat the unit box vertex as a direction, rotate it by the
        // camera orientation only (ignore translation), then place it at the far plane.
        // This keeps the sky centered on the camera without any per-frame camera-follow.
        float3 dir = mul((float3x3)CAMERA_MATRIX, normalize(g.WorldPos));
        g.Normal = dir;
        // Place the box just inside the far plane so it can never tie the water's
        // outer ring (also near zFar) or the far plane itself in the depth buffer.
        g.WorldPos = CAMERA_POSITION + dir * (Z_CLIP_FAR * 0.98);
    }

    SurfaceOutput fragment(SpatialGeometry g) {
        SurfaceOutput o;
        // Unlit: emit the cubemap directly so g_LightEnv doesn't darken the sky.
        o.Emission = cubemap.Sample(sampler, normalize(g.Normal)).rgb;
        o.Albedo = float3(0.0, 0.0, 0.0);
        o.Alpha = 1.0f;
        o.Vertex = g;
        return o;
	}

    typedef UnlitBRDF BRDF;

    UnlitBRDF prepare(SurfaceOutput s) {
        UnlitBRDF brdf;
        return brdf;
	}
};

ENTRY_POINTS(SkyboxMaterial);
