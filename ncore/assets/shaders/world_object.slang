import ncore;

// nc_pipeline: cull=back, fill=solid, blend=alpha, alpha=blend, depth_test=true, depth_write=true

struct ShaderBindings {
    Texture2D Texture;
    SamplerState Sampler;
}

ConstantBuffer<ShaderConstants> g_Constants;
ParameterBlock<ShaderBindings> g_Bindings;

[shader("vertex")]
VSOut VSMain(Vertex3D input, uint instance_id : SV_InstanceID)
{
    float4 worldPos = mul(g_Constants.ModelMatrix, float4(input.Position, 1.0));
    float4 projectedPos = mul(g_Constants.ViewProjMatrix, worldPos);
    float3 worldNormal = mul(float3x3(g_Constants.ModelMatrix), input.Normal);

    VSOut output;
    output.Position = projectedPos;
    output.Normal = worldNormal;
    output.UV = input.UV1;
    output.Color = input.Color;
    return output;
}

[shader("pixel")]
float4 PSMain(VSOut input) : SV_Target
{
    float4 tex_color = g_Bindings.Texture.Sample(g_Bindings.Sampler, input.UV);
    return tex_color * float4(input.Color.rgb, 1.0);
}
