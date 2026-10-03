#include "canvas.slang"

import attributes;

[CullMode(0), Blend(2)]
public struct StandardCanvas : ICanvasMaterial {
    Texture2D texture;
    SamplerState sampler;

    void vertex(inout CanvasGeometry g) {}

    float4 fragment(in CanvasGeometry g) {
        float4 color = texture.Sample(sampler, g.UV) * g.Color;
        color.rgb *= color.a; // pre-multiply alpha
        return color;
    }
};

ENTRY_POINTS(StandardCanvas);
