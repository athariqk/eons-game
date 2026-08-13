import ncore;

ConstantBuffer<ShaderConstants> g_Constants;

[shader("vertex")]
VSOut VSMain(Vertex3D input, uint instance_id : SV_InstanceID)
{
    uint idx = instance_id + 1;
    float3 pos = float3(input.Position.x * idx, input.Position.y, input.Position.z * idx);
    pos.y += sin(pos.x * 5.0f + g_Constants.Time) * 0.5f;

    float4 worldPos = mul(g_Constants.ModelMatrix, float4(pos, 1.0));
    float4 projected = mul(g_Constants.ViewProjMatrix, worldPos);

    VSOut output;
    output.Position = projected;
    output.TexCoord = input.Position;
    output.UV = input.UV1;
    output.Color = input.Color;
    return output;
}

[shader("pixel")]
float4 PSMain(VSOut input) : SV_Target
{
    return input.Color;
}
