#pragma once

#include <ncore/core/vector.h>

namespace sea {

struct WaveCascadeParams {
    nc::Vec2f tile_length    = { 50.0f, 50.0f }; // meters
    float displacement_scale = 1.0f;
    float normal_scale       = 1.0f;

    float wind_speed         = 20.0f; // m/s
    float wind_direction_deg = 0.0f;
    float fetch_length_km    = 550.0f;
    float swell              = 0.8f;
    float spread             = 0.2f; // 0 = Hasselmann, 1 = isotropic
    float detail             = 1.0f; // high-frequency attenuation

    float whitecap    = 0.5f;
    float foam_amount = 5.0f;

    // Runtime (owned/updated by WaveGenerator::update)
    nc::Vec2i spectrum_seed       = { 0, 0 };
    float time                    = 0.0f;
    float foam_grow_rate          = 0.0f;
    float foam_decay_rate         = 0.0f;
    bool should_generate_spectrum = true;
};

// Packed GPU constant buffers — must match Slang cbuffer layouts (16-byte aligned).

struct alignas( 16 ) SpectrumComputeCB {
    int32_t seed_x;
    int32_t seed_y;
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
    uint32_t cascade_index;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas( 16 ) SpectrumModulateCB {
    float tile_length_x;
    float tile_length_y;
    float depth;
    float time;
    uint32_t cascade_index;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas( 16 ) CascadeIndexCB {
    uint32_t cascade_index;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas( 16 ) MapSizeCB {
    uint32_t map_size;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas( 16 ) FftUnpackCB {
    uint32_t cascade_index;
    float whitecap;
    float foam_grow_rate;
    float foam_decay_rate;
};

// Per-cascade scales for the water material: uv_scale.xy, displacement, normal.
struct alignas( 16 ) MapScale {
    float uv_x;
    float uv_y;
    float displacement_scale;
    float normal_scale;
};

} // namespace sea
