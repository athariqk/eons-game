// Memory-efficient coalesced matrix transpose for FFT buffer.
// Port of GodotOceanWaves transpose.glsl

static const uint TILE_SIZE = 32;
static const uint NUM_SPECTRA = 4;
static const uint MAX_MAP_SIZE = 256;

RWStructuredBuffer<float2> fft_buffer;

cbuffer CascadeIndexCB {
    uint cascade_index;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

groupshared float2 tile[TILE_SIZE][TILE_SIZE + 1];

uint data_index(uint cascade, uint pingpong, uint layer, uint y, uint x, uint map_size)
{
    return cascade * map_size * map_size * NUM_SPECTRA * 2 +
           pingpong * NUM_SPECTRA * map_size * map_size +
           layer * map_size * map_size +
           y * map_size +
           x;
}

[shader("compute")]
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CSMain(uint3 global_id : SV_DispatchThreadID, uint3 group_id : SV_GroupID,
            uint3 group_thread_id : SV_GroupThreadID)
{
    uint map_size = MAX_MAP_SIZE;
    uint spectrum = global_id.z;
    if (spectrum >= NUM_SPECTRA)
        return;

    uint2 id_local = group_thread_id.xy;
    uint2 id_block = group_id.xy;

    // Read from ping-pong 1 (FFT output), write to ping-pong 0 for next FFT pass
    uint2 id = global_id.xy;
    if (id.x < map_size && id.y < map_size)
        tile[id_local.y][id_local.x] = fft_buffer[data_index(cascade_index, 1, spectrum, id.y, id.x, map_size)];

    GroupMemoryBarrierWithGroupSync();

    uint2 tid;
    tid.x = id_block.y * TILE_SIZE + id_local.x;
    tid.y = id_block.x * TILE_SIZE + id_local.y;

    if (tid.x < map_size && tid.y < map_size)
        fft_buffer[data_index(cascade_index, 0, spectrum, tid.y, tid.x, map_size)] =
            tile[id_local.x][id_local.y];
}
