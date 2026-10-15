// Unpack IFFT outputs into displacement and normal/foam maps.
// Port of GodotOceanWaves fft_unpack.glsl

static const uint TILE_SIZE = 16;
static const uint NUM_SPECTRA = 4;
static const uint MAX_MAP_SIZE = 512;

RWTexture2DArray<float4> displacement_map;
RWTexture2DArray<float4> normal_map;
RWStructuredBuffer<float2> fft_buffer;

cbuffer FftUnpackCB {
    uint cascade_index;
    float whitecap;
    float foam_grow_rate;
    float foam_decay_rate;
};

uint data_index(uint cascade, uint pingpong, uint layer, uint y, uint x, uint map_size)
{
    // After the second FFT, results live in ping-pong 1.
    return cascade * map_size * map_size * NUM_SPECTRA * 2 +
           pingpong * NUM_SPECTRA * map_size * map_size +
           layer * map_size * map_size +
           y * map_size +
           x;
}

[shader("compute")]
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CSMain(uint3 global_id : SV_DispatchThreadID)
{
    if (global_id.x >= MAX_MAP_SIZE || global_id.y >= MAX_MAP_SIZE)
        return;

    int3 id = int3(int(global_id.x), int(global_id.y), int(cascade_index));

    // ifftshift factor
    float sign_shift = -2.0 * float((global_id.x & 1) ^ (global_id.y & 1)) + 1.0;

    uint i0 = data_index(cascade_index, 1, 0, global_id.y, global_id.x, MAX_MAP_SIZE);
    const uint layer_stride = MAX_MAP_SIZE * MAX_MAP_SIZE;
    float2 s0 = fft_buffer[i0];
    float2 s1 = fft_buffer[i0 + layer_stride];
    float2 s2 = fft_buffer[i0 + 2 * layer_stride];
    float2 s3 = fft_buffer[i0 + 3 * layer_stride];

    // Displacement: (hx, hy, hz)
    displacement_map[id] = float4(s0.x, s0.y, s1.x, 0.0) * sign_shift;

    float dhy_dx = s1.y * sign_shift;
    float dhy_dz = s2.x * sign_shift;
    float dhx_dx = s2.y * sign_shift;
    float dhz_dz = s3.x * sign_shift;
    float dhz_dx = s3.y * sign_shift;

    float jacobian = (1.0 + dhx_dx) * (1.0 + dhz_dz) - dhz_dx * dhz_dx;
    float foam_factor = max(0.0, whitecap - jacobian);   // == -min(0, jacobian - whitecap)
    float foam = normal_map[id].a;
    foam *= exp(-foam_decay_rate);
    foam += foam_factor * foam_grow_rate;
    foam = saturate(foam);

    float2 gradient = float2(dhy_dx, dhy_dz) / (1.0 + abs(float2(dhx_dx, dhz_dz)));
    normal_map[id] = float4(gradient, dhx_dx, foam);
}
