// Coalesced decimation-in-time Stockham FFT.
// Port of GodotOceanWaves fft_compute.glsl
// Source: http://wwwa.pikara.ne.jp/okojisan/otfft-en/stockham3.html (MIT)

static const uint MAX_MAP_SIZE = 256; // must match WaveGenerator map_size
static const uint NUM_SPECTRA = 4;

StructuredBuffer<float4> butterfly;
RWStructuredBuffer<float2> fft_buffer;

cbuffer CascadeIndexCB {
    uint cascade_index;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

groupshared float2 row_shared[2 * MAX_MAP_SIZE];

float2 mul_complex(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

// Buffer layout per cascade:
//   [pingpong 0 | NUM_SPECTRA layers of N*N complex]
//   [pingpong 1 | NUM_SPECTRA layers of N*N complex]
uint data_index(uint cascade, uint pingpong, uint layer, uint y, uint x, uint map_size)
{
    return cascade * map_size * map_size * NUM_SPECTRA * 2 +
           pingpong * NUM_SPECTRA * map_size * map_size +
           layer * map_size * map_size +
           y * map_size +
           x;
}

[shader("compute")]
[numthreads(MAX_MAP_SIZE, 1, 1)]
void CSMain(uint3 global_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID)
{
    uint map_size = MAX_MAP_SIZE;
    uint num_stages = 0;
    uint tmp = map_size;
    while (tmp > 1) {
        num_stages++;
        tmp >>= 1;
    }

    uint col = global_id.x;
    uint row = global_id.y;
    uint spectrum = global_id.z; // 0..3

    if (col >= map_size || row >= map_size || spectrum >= NUM_SPECTRA)
        return;

    // Load row into shared memory (ping-pong 0 is FFT input from modulate / previous transpose)
    row_shared[col] = fft_buffer[data_index(cascade_index, 0, spectrum, row, col, map_size)];

    for (uint stage = 0; stage < num_stages; ++stage) {
        GroupMemoryBarrierWithGroupSync();

        uint read_pp = stage % 2;
        uint write_pp = (stage + 1) % 2;

        float4 butterfly_data = butterfly[stage * map_size + col];
        uint r0 = asuint(butterfly_data.x);
        uint r1 = asuint(butterfly_data.y);
        float2 twiddle = butterfly_data.zw;

        float2 upper = row_shared[read_pp * MAX_MAP_SIZE + r0];
        float2 lower = row_shared[read_pp * MAX_MAP_SIZE + r1];
        row_shared[write_pp * MAX_MAP_SIZE + col] = upper + mul_complex(lower, twiddle);
    }

    GroupMemoryBarrierWithGroupSync();
    // Write to ping-pong 1 (output half)
    fft_buffer[data_index(cascade_index, 1, spectrum, row, col, map_size)] =
        row_shared[(num_stages % 2) * MAX_MAP_SIZE + col];
}
