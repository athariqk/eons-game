// Time-modulate spectrum and pack four complex fields for FFT.
// Port of GodotOceanWaves spectrum_modulate.glsl

static const float PI = 3.141592653589793;
static const float G  = 9.81;
static const uint NUM_SPECTRA = 4;

RWTexture2DArray<float4> spectrum;
RWStructuredBuffer<float2> fft_buffer;

cbuffer SpectrumModulateCB {
    float tile_length_x;
    float tile_length_y;
    float depth;
    float time;
    uint cascade_index;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

float2 exp_complex(float x)
{
    return float2(cos(x), sin(x));
}

float2 mul_complex(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

float2 conj_complex(float2 x)
{
    return float2(x.x, -x.y);
}

float dispersion_relation(float k)
{
    return sqrt(G * k * tanh(k * depth));
}

uint fft_index(uint cascade, uint layer, uint y, uint x, uint map_size)
{
    // Layout: [cascade][pingpong=0 for modulate output][layer][y][x]
    // Matches Godot FFT_DATA with write into the "in" half used by first FFT pass.
    return cascade * map_size * map_size * NUM_SPECTRA * 2 +
           layer * map_size * map_size +
           y * map_size +
           x;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void CSMain(uint3 global_id : SV_DispatchThreadID)
{
    uint width, height, layers;
    spectrum.GetDimensions(width, height, layers);
    uint map_size = width;

    if (global_id.x >= map_size || global_id.y >= map_size)
        return;

    int3 id = int3(global_id.xy, cascade_index);

    float2 tile_length = float2(tile_length_x, tile_length_y);
    float2 k_vec = (float2(id.xy) - float2(map_size, map_size) * 0.5) * (2.0 * PI) / tile_length;
    float k = length(k_vec) + 1e-6;
    float2 k_unit = k_vec / k;

    float4 h0 = spectrum[id]; // xy = h0(k), zw = conj(h0(-k))
    float dispersion = dispersion_relation(k) * time;
    float2 modulation = exp_complex(dispersion);
    float2 h = mul_complex(h0.xy, modulation) + mul_complex(h0.zw, conj_complex(modulation));
    float2 h_inv = float2(-h.y, h.x);

    float2 hx = h_inv * k_unit.y;
    float2 hy = h;
    float2 hz = h_inv * k_unit.x;

    float2 dhy_dx = h_inv * k_vec.y;
    float2 dhy_dz = h_inv * k_vec.x;
    float2 dhx_dx = -h * k_vec.y * k_unit.y;
    float2 dhz_dz = -h * k_vec.x * k_unit.x;
    float2 dhz_dx = -h * k_vec.y * k_unit.x;

    uint x = global_id.x;
    uint y = global_id.y;
    uint c = cascade_index;

    fft_buffer[fft_index(c, 0, y, x, map_size)] = float2(hx.x - hy.y, hx.y + hy.x);
    fft_buffer[fft_index(c, 1, y, x, map_size)] = float2(hz.x - dhy_dx.y, hz.y + dhy_dx.x);
    fft_buffer[fft_index(c, 2, y, x, map_size)] = float2(dhy_dz.x - dhx_dx.y, dhy_dz.y + dhx_dx.x);
    fft_buffer[fft_index(c, 3, y, x, map_size)] = float2(dhz_dz.x - dhz_dx.y, dhz_dz.y + dhz_dx.x);
}
