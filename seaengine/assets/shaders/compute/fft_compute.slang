// Fused radix-2 / radix-4 Stockham FFT (inverse direction, unnormalized), one row per group.
// Drop-in replacement for the radix-2 table-driven fft_compute.slang:
//   - same dispatch: (1, map_size, NUM_SPECTRA) groups, same buffer layout, same results
//   - 128 threads per row instead of 512 (each thread owns 4 outputs per pass)
//   - 9 radix-2 stages -> 1 radix-2 stage (fused with the global load) + 4 radix-4 passes
//   - the last pass writes straight to global memory (no shared round trip)
//   - no butterfly table: twiddles come from a 128-entry LUT built in shared memory
// Derivation: two consecutive radix-2 Stockham stages (stride s, 2s) close over 4 inputs
// x_k = in[t + (N/4)*k]; see comments in the pass loop. Checked against numpy ifft*N.
// Algorithm family: http://wwwa.pikara.ne.jp/okojisan/otfft-en/stockham3.html (MIT)

static const uint MAX_MAP_SIZE   = 512;                  // must match WaveGenerator map_size
static const uint LOG2N          = 9;                    // log2(MAX_MAP_SIZE); must be ODD
static const uint NUM_SPECTRA    = 4;
static const uint NUM_THREADS    = MAX_MAP_SIZE / 4;     // one radix-4 butterfly per thread
static const uint NUM_R4_PASSES  = (LOG2N - 1) / 2;
static const float TWO_PI        = 6.283185307179586;

RWStructuredBuffer<float2> fft_buffer;

cbuffer CascadeIndexCB {
    uint cascade_index;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

groupshared float2 row_shared[2 * MAX_MAP_SIZE];
groupshared float2 twiddle_lut[NUM_THREADS];   // exp(2*pi*i*m/N), m < N/4

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
[numthreads(NUM_THREADS, 1, 1)]
void CSMain(uint3 group_id : SV_GroupID, uint3 group_thread_id : SV_GroupThreadID)
{
    uint t        = group_thread_id.x;   // 0 .. N/4-1
    uint row      = group_id.y;
    uint spectrum = group_id.z;          // 0..3

    // Group-uniform, so returning before the barriers is safe.
    if (row >= MAX_MAP_SIZE || spectrum >= NUM_SPECTRA)
        return;

    uint in_base  = data_index(cascade_index, 0, spectrum, row, 0, MAX_MAP_SIZE);
    uint out_base = data_index(cascade_index, 1, spectrum, row, 0, MAX_MAP_SIZE);

    // Twiddle LUT: one entry per thread.
    float angle = TWO_PI * float(t) / float(MAX_MAP_SIZE);
    twiddle_lut[t] = float2(cos(angle), sin(angle));

    // Load 4 coalesced elements and do radix-2 stage 0 (twiddle == 1) in registers.
    // Stage 0 butterfly i reads in[i], in[i + N/2] and writes out[2i], out[2i + 1].
    float2 in0 = fft_buffer[in_base + t];
    float2 in1 = fft_buffer[in_base + t + NUM_THREADS];
    float2 in2 = fft_buffer[in_base + t + 2 * NUM_THREADS];
    float2 in3 = fft_buffer[in_base + t + 3 * NUM_THREADS];

    row_shared[2 * t]                     = in0 + in2;   // butterfly i = t
    row_shared[2 * t + 1]                 = in0 - in2;
    row_shared[2 * (t + NUM_THREADS)]     = in1 + in3;   // butterfly i = t + N/4
    row_shared[2 * (t + NUM_THREADS) + 1] = in1 - in3;

    // Radix-4 passes cover the remaining stages in pairs: (1,2), (3,4), (5,6), (7,8).
    [unroll]
    for (uint q = 0; q < NUM_R4_PASSES; ++q) {
        const uint ls = 1 + 2 * q;           // log2 of the stride s of the pair's first stage
        const uint s  = 1u << ls;
        const uint rd = (q & 1) * MAX_MAP_SIZE;
        const uint wr = ((q + 1) & 1) * MAX_MAP_SIZE;

        GroupMemoryBarrierWithGroupSync();   // LUT + previous writes visible; previous reads done

        uint j  = t & (s - 1);
        uint ip = t >> ls;

        // Inputs are always t + (N/4)*k because s*ip + j == t.
        float2 x0 = row_shared[rd + t];
        float2 x1 = row_shared[rd + t + NUM_THREADS];
        float2 x2 = row_shared[rd + t + 2 * NUM_THREADS];
        float2 x3 = row_shared[rd + t + 3 * NUM_THREADS];

        // T = exp(i*pi*j/(2s)) = LUT[j * N/(4s)].  T^2 and T^3 by multiplication.
        float2 T  = twiddle_lut[j << (LOG2N - 2 - ls)];
        float2 T2 = mul_complex(T, T);
        float2 T3 = mul_complex(T2, T);

        float2 b = mul_complex(T2, x2);
        float2 c = mul_complex(T,  x1);
        float2 d = mul_complex(T3, x3);

        float2 u0 = x0 + b;
        float2 u1 = x0 - b;
        float2 p  = c + d;
        float2 r  = c - d;
        float2 ir = float2(-r.y, r.x);       // i * r

        float2 y0 = u0 + p;
        float2 y1 = u0 - p;
        float2 y2 = u1 + ir;
        float2 y3 = u1 - ir;

        uint o = 4 * s * ip + j;
        if (q == NUM_R4_PASSES - 1) {
            // Last pass: ip == 0, so o == j and each store is contiguous across threads.
            fft_buffer[out_base + o]         = y0;
            fft_buffer[out_base + o + 2 * s] = y1;
            fft_buffer[out_base + o + s]     = y2;
            fft_buffer[out_base + o + 3 * s] = y3;
        } else {
            row_shared[wr + o]         = y0;
            row_shared[wr + o + 2 * s] = y1;
            row_shared[wr + o + s]     = y2;
            row_shared[wr + o + 3 * s] = y3;
        }
    }
}
