// Precomputes butterfly factors for a Stockham FFT kernel.
// Port of GodotOceanWaves fft_butterfly.glsl

static const float PI = 3.141592653589793;

RWStructuredBuffer<float4> butterfly;

cbuffer MapSizeCB {
    uint map_size;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

float2 exp_complex(float x)
{
    return float2(cos(x), sin(x));
}

[shader("compute")]
[numthreads(64, 1, 1)]
void CSMain(uint3 global_id : SV_DispatchThreadID)
{
    uint col = global_id.x;   // 0 .. map_size/2 - 1
    uint stage = global_id.y; // 0 .. log2(map_size) - 1

    if (col >= map_size / 2)
        return;

    uint stride = 1u << stage;
    uint mid = map_size >> (stage + 1);
    uint i = col >> stage;
    uint j = col % stride;

    float2 twiddle_factor = exp_complex(PI / float(stride) * float(j));
    uint r0 = stride * (i + 0) + j;
    uint r1 = stride * (i + mid) + j;
    uint w0 = stride * (2 * i + 0) + j;
    uint w1 = stride * (2 * i + 1) + j;

    float2 read_indices = float2(asfloat(r0), asfloat(r1));

    butterfly[stage * map_size + w0] = float4(read_indices, twiddle_factor);
    butterfly[stage * map_size + w1] = float4(read_indices, -twiddle_factor);
}
