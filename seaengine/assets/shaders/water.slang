import ncore;

ConstantBuffer<ShaderConstants> g_Constants;

[shader("vertex")]
VSOut VSMain(Vertex3D input, uint instance_id : SV_InstanceID)
{
    uint idx = instance_id + 1;
    float3 pos = float3(input.Position.x * idx, input.Position.y, input.Position.z * idx);
    pos.y += sin(pos.x * 5.0f + g_Constants.Time) * 0.5f;
    pos.y += cos(pos.z * 3.0f + g_Constants.Time) * 0.3f;

    float4 worldPos = mul(g_Constants.ModelMatrix, float4(pos, 1.0));
    float4 projected = mul(g_Constants.ViewProjMatrix, worldPos);

    float dDdx = 2.5 * cos(pos.x * 5.0f + g_Constants.Time);
    float dDdz = 0.9 * sin(pos.z * 3.0f + g_Constants.Time);
    float3 normal = normalize(float3(-dDdx, 1.0, -dDdz));
    float3 worldNormal = normalize(mul(float3x3(g_Constants.ModelMatrixInv), normal));

    VSOut output;
    output.Position = projected;
    output.TexCoord = input.Position;
    output.Normal = worldNormal;
    output.UV = input.UV1;
    output.Color = input.Color;
    return output;
}

[shader("pixel")]
float4 PSMain(VSOut input) : SV_Target
{
    float3 cameraPos = g_Constants.CameraMatrix._14_24_34;
    float p = dot(input.Normal, cameraPos);

    float3 debugColor = (input.Normal * 0.5) + 0.5;
    return float4(debugColor, 1.0);
}
