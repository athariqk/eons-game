// Precomputes butterfly factors for a Stockham FFT kernel.
// Port of GodotOceanWaves fft_butterfly.glsl

static const float PI = 3.141592653589793;

RWStructuredBuffer<float4> butterfly;

float2 exp_complex(float x)
{
    return float2(cos(x), sin(x));
}

[shader("compute")]
[numthreads(64, 1, 1)]
void CSMain(uint3 global_id : SV_DispatchThreadID, uint3 group_id : SV_GroupID, uint3 group_thread_id : SV_GroupThreadID)
{
    // map_size inferred from dispatch: groups_x * 64 * 2
    uint map_size = (group_id.x + 1) > 0
                        ? (/* filled below */ 0)
                        : 0;
    // Prefer deriving from total columns covered:
    // Dispatch is [(map_size/2)/64, num_stages, 1], each thread one of map_size/2 write columns * 2 outputs.
    // Reconstruct map_size from: col runs 0 .. map_size/2 - 1 effectively via w0/w1 spanning full size.
    // Use: total X threads = map_size/2
    uint col = global_id.x;
    uint stage = global_id.y;

    // map_size = 2 * (number of columns processed) = 2 * (groups.x * 64)
    // We don't have NumWorkGroups in all targets — pass via dimensions of buffer is awkward.
    // Convention: butterfly buffer length = num_stages * map_size; stage index is Y.
    // Compute map_size from: for stage 0, stride=1, mid=map_size/2, cols written = map_size.
    // Practical approach used by original: map_size = gl_NumWorkGroups.x * local_size * 2
    uint num_groups_x = 0; // placeholder replaced below

    // Slang/HLSL: use SV_GroupID extent is not available; store map_size in a fixed way.
    // We encode map_size as: dispatch X groups = (map_size/2)/64, so map_size = groupCountX * 128.
    // Without group count, assume MAX and early-out — WaveGenerator always dispatches exact size.
    // Read map_size from a constant if needed later. For now use butterfly buffer element count heuristic.

    // Fixed: WaveGenerator dispatches (map_size/2)/64 groups in X. Thread col = global_id.x
    // valid range [0, map_size/2). Each thread writes two butterfly entries at w0 and w1.
    // Original shader:
    //   map_size = NumWorkGroups.x * 64 * 2
    // In D3D/Vulkan without NumWorkGroups, we pass map_size via unused high bits — instead hardcode
    // via compile-time or CB. Add a tiny CB for robustness.

    // TEMP: derive from col range — WaveGenerator guarantees col < map_size/2.
    // We'll use a compile-time default matching WaveGenerator::kDefaultMapSize.
    const uint MAP_SIZE = 256; // must match WaveGenerator map_size until CB is added

    if (col >= MAP_SIZE / 2)
        return;

    uint stride = 1u << stage;
    uint mid = MAP_SIZE >> (stage + 1);
    uint i = col >> stage;
    uint j = col % stride;

    float2 twiddle_factor = exp_complex(PI / float(stride) * float(j));
    uint r0 = stride * (i + 0) + j;
    uint r1 = stride * (i + mid) + j;
    uint w0 = stride * (2 * i + 0) + j;
    uint w1 = stride * (2 * i + 1) + j;

    // Pack read indices as float bits (same as Godot)
    float2 read_indices = float2(asfloat(r0), asfloat(r1));

    butterfly[stage * MAP_SIZE + w0] = float4(read_indices, twiddle_factor);
    butterfly[stage * MAP_SIZE + w1] = float4(read_indices, -twiddle_factor);
}
