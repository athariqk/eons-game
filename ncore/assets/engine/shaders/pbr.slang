import common;

struct ShaderBindings {
    Texture2D Texture;
    SamplerState Sampler;
}

ConstantBuffer<ShaderConstants> g_Constants;
ParameterBlock<ShaderBindings> g_Bindings;

[shader("vertex")]
VertexOut vertex(Vertex3DIn input, uint instance_id : SV_InstanceID)
{
    VertexOut output;
    output.Color = input.Color;
    output.UV = input.UV1;
    output.Position = mul(g_Constants.ModelMatrix, float4(input.Position, 1.0));
    output.Position = mul(g_Constants.ViewProjMatrix, output.Position);
    return output;
}

[shader("pixel")]
float4 fragment(PixelIn input) : SV_Target
{
    return float4(input.Color.rgb, 1.0);
}
