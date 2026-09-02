// Unpack IFFT outputs into displacement and normal/foam maps.
// Port of GodotOceanWaves fft_unpack.glsl

static const uint TILE_SIZE = 16;
static const uint NUM_SPECTRA = 4;
static const uint MAX_MAP_SIZE = 256;

RWTexture2DArray<float4> displacement_map;
RWTexture2DArray<float4> normal_map;
RWStructuredBuffer<float2> fft_buffer;

cbuffer FftUnpackCB {
    uint cascade_index;
    float whitecap;
    float foam_grow_rate;
    float foam_decay_rate;
};

groupshared float2 tile[NUM_SPECTRA][TILE_SIZE][TILE_SIZE];

uint data_index(uint cascade, uint pingpong, uint layer, uint y, uint x, uint map_size)
{
    // After second FFT, results live in ping-pong 1 (same as Godot NUM_SPECTRA*N*N offset).
    return cascade * map_size * map_size * NUM_SPECTRA * 2 +
           pingpong * NUM_SPECTRA * map_size * map_size +
           layer * map_size * map_size +
           y * map_size +
           x;
}

[shader("compute")]
[numthreads(TILE_SIZE, TILE_SIZE, 2)]
void CSMain(uint3 global_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID)
{
    uint map_size = MAX_MAP_SIZE;
    uint3 id_local = group_thread_id;
    int3 id = int3(int(global_id.x), int(global_id.y), int(cascade_index));

    if (global_id.x >= map_size || global_id.y >= map_size)
        return;

    // ifftshift factor
    float sign_shift = -2.0 * float((global_id.x & 1) ^ (global_id.y & 1)) + 1.0;

    // Load two spectra layers per Z slice into shared memory
    tile[id_local.z * 2][id_local.y][id_local.x] =
        fft_buffer[data_index(cascade_index, 1, id_local.z * 2, global_id.y, global_id.x, map_size)];
    tile[id_local.z * 2 + 1][id_local.y][id_local.x] =
        fft_buffer[data_index(cascade_index, 1, id_local.z * 2 + 1, global_id.y, global_id.x, map_size)];

    GroupMemoryBarrierWithGroupSync();

    if (id_local.z == 0) {
        float hx = tile[0][id_local.y][id_local.x].x;
        float hy = tile[0][id_local.y][id_local.x].y;
        float hz = tile[1][id_local.y][id_local.x].x;
        displacement_map[id] = float4(hx, hy, hz, 0.0) * sign_shift;
    } else {
        float dhy_dx = tile[1][id_local.y][id_local.x].y * sign_shift;
        float dhy_dz = tile[2][id_local.y][id_local.x].x * sign_shift;
        float dhx_dx = tile[2][id_local.y][id_local.x].y * sign_shift;
        float dhz_dz = tile[3][id_local.y][id_local.x].x * sign_shift;
        float dhz_dx = tile[3][id_local.y][id_local.x].y * sign_shift;

        float jacobian = (1.0 + dhx_dx) * (1.0 + dhz_dz) - dhz_dx * dhz_dx;
        float foam_factor = -min(0.0, jacobian - whitecap);
        float foam = normal_map[id].a;
        foam *= exp(-foam_decay_rate);
        foam += foam_factor * foam_grow_rate;
        foam = saturate(foam);

        float2 gradient = float2(dhy_dx, dhy_dz) / (1.0 + abs(float2(dhx_dx, dhz_dz)));
        normal_map[id] = float4(gradient, dhx_dx, foam);
    }
}
