// Diagnostic fallback drawn for meshes that resolve to no usable material.
//
// Deliberately unlit and parameterless: a lit material would depend on the light
// environment and on vertex colour, both of which can silently produce black and
// hide the problem this shader is meant to expose.
#include "spatial.slang"

import attributes;

[CullMode(0), DepthTest(1), DepthWrite(1), Blend(0)]
public struct MissingMaterial : ISpatialMaterial
{
    void vertex(inout SpatialGeometry g) {}

    SurfaceOutput fragment(in SpatialGeometry g)
    {
        SurfaceOutput o;
        o.Albedo    = float3(1.0, 0.0, 1.0);
        o.Emission  = float3(1.0, 0.0, 1.0); // spatial.slang adds emission on top of the BRDF
        o.Metallic  = 0.0;
        o.Roughness = 1.0;
        o.Alpha     = 1.0;
        o.Vertex    = g;
        return o;
    }

    typedef UnlitBRDF BRDF;

    UnlitBRDF prepare(SurfaceOutput s)
    {
        return UnlitBRDF();
    }
};

ENTRY_POINTS(MissingMaterial);
