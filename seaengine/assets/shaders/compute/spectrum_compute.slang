// Generates initial JONSWAP/TMA spectra with Hasselmann directional spread.
// Port of GodotOceanWaves assets/shaders/compute/spectrum_compute.glsl
// Sources: Tessendorf; Horvath — Empirical Directional Wave Spectra

static const float PI = 3.141592653589793;
static const float G  = 9.81;

RWTexture2DArray<float4> spectrum;

cbuffer SpectrumComputeCB {
    int seed_x;
    int seed_y;
    float tile_length_x;
    float tile_length_y;
    float alpha;
    float peak_frequency;
    float wind_speed;
    float angle_rad;
    float depth;
    float swell;
    float detail;
    float spread;
    uint cascade_index;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

float2 hash(uint2 x)
{
    uint h32 = x.y + 374761393u + x.x * 3266489917u;
    h32 = 2246822519u * (h32 ^ (h32 >> 15));
    h32 = 3266489917u * (h32 ^ (h32 >> 13));
    uint n = h32 ^ (h32 >> 16);
    uint2 rz = uint2(n, n * 48271u);
    return float2((rz.xy >> 1) & uint2(0x7FFFFFFFu, 0x7FFFFFFFu)) / float(0x7FFFFFFF);
}

float2 gaussian(float2 x)
{
    float r = sqrt(-2.0 * log(max(x.x, 1e-9)));
    float theta = 2.0 * PI * x.y;
    return float2(r * cos(theta), r * sin(theta));
}

float2 conj_complex(float2 x)
{
    return float2(x.x, -x.y);
}

float2 dispersion_relation(float k)
{
    float a = k * depth;
    float b = tanh(a);
    float dispersion = sqrt(G * k * b);
    float d_dispersion = 0.5 * G * (b + a * (1.0 - b * b)) / max(dispersion, 1e-6);
    return float2(dispersion, d_dispersion);
}

float longuet_higgins_normalization(float s)
{
    float a = sqrt(s);
    return (s < 0.4)
               ? (0.5 / PI) + s * (0.220636 + s * (-0.109 + s * 0.090))
               : rsqrt(PI) * (a * 0.5 + (1.0 / a) * 0.0625);
}

float longuet_higgins_function(float s, float theta)
{
    return longuet_higgins_normalization(s) * pow(abs(cos(theta * 0.5)), 2.0 * s);
}

float hasselmann_directional_spread(float w, float w_p, float wind_spd, float theta)
{
    float p = w / w_p;
    float s = (w <= w_p) ? 6.97 * pow(abs(p), 4.06)
                         : 9.77 * pow(abs(p), -2.33 - 1.45 * (wind_spd * w_p / G - 1.17));
    float s_xi = 16.0 * tanh(w_p / w) * swell * swell;
    return longuet_higgins_function(s + s_xi, theta - angle_rad);
}

float TMA_spectrum(float w, float w_p, float alpha_in)
{
    const float beta = 1.25;
    const float gamma = 3.3;

    float sigma = (w <= w_p) ? 0.07 : 0.09;
    float r = exp(-(w - w_p) * (w - w_p) / (2.0 * sigma * sigma * w_p * w_p));
    float jonswap = (alpha_in * G * G) / pow(w, 5.0) * exp(-beta * pow(w_p / w, 4.0)) * pow(gamma, r);

    float w_h = min(w * sqrt(depth / G), 2.0);
    float depth_att = (w_h <= 1.0) ? 0.5 * w_h * w_h : 1.0 - 0.5 * (2.0 - w_h) * (2.0 - w_h);
    return jonswap * depth_att;
}

float2 get_spectrum_amplitude(int2 id, int2 map_size)
{
    float2 tile_length = float2(tile_length_x, tile_length_y);
    float2 dk = 2.0 * PI / tile_length;
    float2 k_vec = (float2(id) - float2(map_size) * 0.5) * dk;
    float k = length(k_vec) + 1e-6;
    float theta = atan2(k_vec.x, k_vec.y);

    float2 dispersion = dispersion_relation(k);
    float w = dispersion[0];
    float w_norm = dispersion[1] / k * dk.x * dk.y;
    float s = TMA_spectrum(w, peak_frequency, alpha);
    float d = lerp(0.5 / PI,
                   hasselmann_directional_spread(w, peak_frequency, wind_speed, theta),
                   1.0 - spread) *
              exp(-(1.0 - detail) * (1.0 - detail) * k * k);

    int2 seed = int2(seed_x, seed_y);
    return gaussian(hash(uint2(id + seed))) * sqrt(max(2.0 * s * d * w_norm, 0.0));
}

[shader("compute")]
[numthreads(16, 16, 1)]
void CSMain(uint3 global_id : SV_DispatchThreadID)
{
    uint width, height, layers;
    spectrum.GetDimensions(width, height, layers);

    if (global_id.x >= width || global_id.y >= height)
        return;

    int2 dims = int2(int(width), int(height));
    int3 id = int3(int(global_id.x), int(global_id.y), int(cascade_index));
    int2 id0 = id.xy;
    int2 id1;
    id1.x = ((-id0.x) % dims.x + dims.x) % dims.x;
    id1.y = ((-id0.y) % dims.y + dims.y) % dims.y;

    float2 h_k = get_spectrum_amplitude(id0, dims);
    float2 h_mk = conj_complex(get_spectrum_amplitude(id1, dims));
    spectrum[id] = float4(h_k, h_mk);
}
