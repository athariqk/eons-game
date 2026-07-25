import common;

ConstantBuffer<Constants> constants;

struct VertexInput
{
    float2 pos;
    float2 uv : TEXCOORD1;
    float4 col : TEXCOORD2;
};

struct VertexOutput
{
    float4 col : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 pos : SV_Position;
};

struct PixelInput
{
    float4 col : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

struct Bindings {
	Texture2D Texture;
	SamplerState Sampler;
}

ParameterBlock<Bindings> bindings;

[shader("vertex")]
VertexOutput vertex(VertexInput input)
{
    VertexOutput output;
    output.col = input.col;
    output.uv = input.uv;
    output.pos = mul(constants.ProjectionMatrix, float4(input.pos, 0.0, 1.0));
    return output;
}

[shader("pixel")]
float4 fragment(PixelInput input) : SV_Target
{
    float4 color = bindings.Texture.Sample(bindings.Sampler, input.uv) * input.col;

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
