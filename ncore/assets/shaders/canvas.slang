import ncore;

struct ShaderBindings {
	Texture2D Texture;
	SamplerState Sampler;
}

ConstantBuffer<ShaderConstants> g_Constants;
ParameterBlock<ShaderBindings> g_Bindings;

[shader("vertex")]
VSOut VSMain(Vertex2D input)
{
    float4 screenPos = mul(g_Constants.ViewProjMatrix, float4(input.Position, 0.0, 1.0));

    VSOut output;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Position = screenPos;
    return output;
}

[shader("pixel")]
float4 PSMain(VSOut input) : SV_Target
{
    float4 color = g_Bindings.Texture.Sample(g_Bindings.Sampler, input.UV) * input.Color;
	color.rgb *= color.a; // pre-multiply alpha
    return color;
}
