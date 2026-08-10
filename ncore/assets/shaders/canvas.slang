import ncore;

struct ShaderBindings {
	Texture2D Texture;
	SamplerState Sampler;
}

ConstantBuffer<ShaderConstants> g_Constants;
ParameterBlock<ShaderBindings> g_Bindings;

[shader("vertex")]
VertexOut vertex(Vertex2DIn input)
{
    VertexOut output;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Position = mul(g_Constants.ViewProjMatrix, float4(input.Position, 0.0, 1.0));
    return output;
}

[shader("pixel")]
float4 fragment(PixelIn input) : SV_Target
{
    float4 color = g_Bindings.Texture.Sample(g_Bindings.Sampler, input.UV) * input.Color;

    // Premultiply alpha
    color.rgb *= color.a;

    // Per-channel sRGB -> linear decode (gamma correction)
    color.r = (color.r < 0.0404500008f)
        ? (color.r / 12.92f)
        : pow(max(color.r + 0.0549999997f, 0.0f) / 1.05499995f, 2.4f);

    color.g = (color.g < 0.0404500008f)
        ? (color.g / 12.92f)
        : pow(max(color.g + 0.0549999997f, 0.0f) / 1.05499995f, 2.4f);

    color.b = (color.b < 0.0404500008f)
        ? (color.b / 12.92f)
        : pow(max(color.b + 0.0549999997f, 0.0f) / 1.05499995f, 2.4f);

    // Alpha channel decoded via its complement (1 - alpha), then re-inverted
    float inv_a = 1.0f - color.a;
    inv_a = (inv_a < 0.0404500008f)
        ? (inv_a / 12.92f)
        : pow(max(inv_a + 0.0549999997f, 0.0f) / 1.05499995f, 2.4f);
    color.a = 1.0f - inv_a;

    return color;
}
