import ncore;

struct ShaderBindings {
    TextureCube Texture;
    SamplerState Sampler;
};

ConstantBuffer<ShaderConstants> g_Constants;
ParameterBlock<ShaderBindings> g_Bindings;

[shader("vertex")]
VSOut vertex(Vertex3D input)
{
    float3 cameraPos = g_Constants.CameraMatrix._14_24_34;
    float4 worldPos = float4(input.Position + cameraPos, 1.0);
    float4 projectedPos = mul(g_Constants.ViewProjMatrix, worldPos);

    VSOut output;
    output.Position = projectedPos;
    output.TexCoord = input.Position;
    return output;
}

[shader("pixel")]
float4 fragment(VSOut input) : SV_Target
{
    float3 dir = normalize(input.TexCoord);
    float3 texColor = g_Bindings.Texture.Sample(g_Bindings.Sampler, dir).rgb;   
    return float4(texColor, 1.0);
}
