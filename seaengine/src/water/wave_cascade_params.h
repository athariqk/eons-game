#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/vector.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace sea {

using namespace nc;

// Per-cascade scales for the water material: uv_scale.xy, displacement, normal.
struct REFLECT MapScale {
    REFLECT Vec2f tile_length        = { 50.0f, 50.0f }; // meters
    REFLECT float displacement_scale = 1.0f;
    REFLECT float normal_scale       = 1.0f;
};

struct REFLECT WaveCascadeParams {
    REFLECT MapScale scale           = {};
    REFLECT float wind_speed         = 20.0f; // m/s
    REFLECT float wind_direction_deg = 0.0f;
    REFLECT float fetch_length_km    = 550.0f;
    REFLECT float swell              = 0.8f;
    REFLECT float spread             = 0.2f; // 0 = Hasselmann, 1 = isotropic
    REFLECT float detail             = 1.0f; // high-frequency attenuation
    REFLECT float whitecap           = 0.5f;
    REFLECT float foam_amount        = 5.0f;

    // Runtime (owned/updated by WaveGenerator::update)
    Vec2i spectrum_seed           = { 0, 0 };
    float time                    = 0.0f;
    float foam_grow_rate          = 0.0f;
    float foam_decay_rate         = 0.0f;
    bool should_generate_spectrum = true;
};

NC_COMPONENT( WaterSimComponent )
{
    static constexpr uint32_t MAX_CASCADES = 8;

    REFLECT int cascade_count = 0;
    REFLECT WaveCascadeParams cascades[MAX_CASCADES];
};

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

} // namespace sea
