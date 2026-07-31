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
    float3 offset = random_point_in_sphere(instance_id) * 10;

    VertexOut output;
    output.Color = input.Color;
    output.UV = input.UV1;
    output.Position = mul(g_Constants.ModelMatrix, float4(input.Position, 1.0)) + float4(offset, 1.0);
    output.Position = mul(g_Constants.ViewMatrix, output.Position);
    return output;
}

[shader("pixel")]
float4 fragment(PixelIn input) : SV_Target
{
    return float4(input.Color.rgb, 1.0);
}
