// Coalesced matrix transpose for the FFT buffer.
// Port of GodotOceanWaves transpose.glsl.

static const uint TILE_SIZE   = 32;
static const uint BLOCK_ROWS  = 8;
static const uint NUM_SPECTRA = 4;
static const uint MAX_MAP_SIZE = 512;

RWStructuredBuffer<float2> fft_buffer;

cbuffer CascadeIndexCB {
    uint cascade_index;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

groupshared float2 tile[TILE_SIZE][TILE_SIZE + 1];   // +1 avoids bank conflicts

uint data_index(uint cascade, uint pingpong, uint layer, uint y, uint x, uint map_size)
{
    return cascade * map_size * map_size * NUM_SPECTRA * 2 +
           pingpong * NUM_SPECTRA * map_size * map_size +
           layer * map_size * map_size +
           y * map_size +
           x;
}

[shader("compute")]
[numthreads(TILE_SIZE, BLOCK_ROWS, 1)]
void CSMain(uint3 group_id : SV_GroupID, uint3 group_thread_id : SV_GroupThreadID)
{
    uint spectrum = group_id.z;
    if (spectrum >= NUM_SPECTRA)
        return;

    uint lx = group_thread_id.x;
    uint ly = group_thread_id.y;

    // Read from ping-pong 1 (FFT output), write to ping-pong 0 for the next FFT pass.
    uint x  = group_id.x * TILE_SIZE + lx;
    uint y0 = group_id.y * TILE_SIZE + ly;

    [unroll]
    for (uint r = 0; r < TILE_SIZE; r += BLOCK_ROWS) {
        if (x < MAX_MAP_SIZE && (y0 + r) < MAX_MAP_SIZE)
            tile[ly + r][lx] = fft_buffer[data_index(cascade_index, 1, spectrum, y0 + r, x, MAX_MAP_SIZE)];
    }

    GroupMemoryBarrierWithGroupSync();

    // Transposed block: (group_id.y, group_id.x)
    uint ox  = group_id.y * TILE_SIZE + lx;
    uint oy0 = group_id.x * TILE_SIZE + ly;

    [unroll]
    for (uint r2 = 0; r2 < TILE_SIZE; r2 += BLOCK_ROWS) {
        if (ox < MAX_MAP_SIZE && (oy0 + r2) < MAX_MAP_SIZE)
            fft_buffer[data_index(cascade_index, 0, spectrum, oy0 + r2, ox, MAX_MAP_SIZE)] =
                tile[lx][ly + r2];
    }
}
