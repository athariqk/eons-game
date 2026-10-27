#pragma once

#include <ncore/core/rid.h>
#include <ncore/services/video/render_service.h>

#include "wave_cascade_params.h"

namespace nc {
class ResourceLoader;
}

namespace sea {

/**
 * GPU wave spectra + Stockham FFT pipeline (port of 2Retr0/GodotOceanWaves).
 *
 * Owns cascade Texture2DArrays (spectrum / displacement / normal), FFT buffers,
 * and compute PSOs. Call init_gpu() once, then update() each sim tick.
 *
 * map_size must match MAX_MAP_SIZE in fft_compute/transpose/fft_unpack.slang.
 */
class WaveGenerator : nc::Object {
    NCLASS( WaveGenerator, nc::Object )

public:
    static constexpr float GRAVITY             = 9.81f;
    static constexpr float DEPTH               = 20.0f;
    static constexpr uint32_t NUM_SPECTRA      = 4;
    static constexpr uint32_t DEFAULT_MAP_SIZE = 512;

    static constexpr uint32_t WAVE_GEN_TS_SLOT       = GfxInterface::MAX_TIMESTAMP_SLOTS - 1;
    static constexpr uint32_t WAVE_MODULATE_TS_SLOT  = WAVE_GEN_TS_SLOT - 1;
    static constexpr uint32_t WAVE_FF1_TS_SLOT       = WAVE_MODULATE_TS_SLOT - 1;
    static constexpr uint32_t WAVE_TRANSPOSE_TS_SLOT = WAVE_FF1_TS_SLOT - 1;
    static constexpr uint32_t WAVE_FFT2_TS_SLOT      = WAVE_TRANSPOSE_TS_SLOT - 1;
    static constexpr uint32_t WAVE_UNPACK_TS_SLOT    = WAVE_FFT2_TS_SLOT - 1;

    WaveGenerator() = default;
    ~WaveGenerator();

    WaveGenerator( const WaveGenerator& )            = delete;
    WaveGenerator& operator=( const WaveGenerator& ) = delete;

    void init_gpu(
        nc::RenderService& p_renderer, nc::ResourceLoader& p_resources, uint32_t p_map_size, uint32_t p_num_cascades
    );
    void shutdown( nc::RenderService& p_renderer );
    void update( nc::RenderService& p_renderer, float p_delta, nc::Span<WaveCascadeParams> p_cascades );
    void flush( nc::RenderService& p_renderer );

    bool get_initialized() const
    {
        return initialized;
    }
    uint32_t get_map_size() const
    {
        return map_size;
    }
    uint32_t get_num_cascades() const
    {
        return num_cascades;
    }

    nc::RID get_displacement_map() const
    {
        return displacement_map;
    }
    nc::RID get_normal_map() const
    {
        return normal_map;
    }

    static float jonswap_alpha( float wind_speed, float fetch_meters );
    static float jonswap_peak_angular_frequency( float wind_speed, float fetch_meters );

private:
    void create_resources( nc::RenderService& renderer );
    void create_pipelines( nc::RenderService& renderer, nc::ResourceLoader& resources );
    void destroy_resources( nc::RenderService& renderer );
    void dispatch_cascade( nc::RenderService& renderer, uint32_t cascade_index, nc::Span<WaveCascadeParams> cascades );
    void write_cb( nc::RenderService& renderer, nc::RID buffer, const void* data, size_t size );

    bool initialized        = false;
    uint32_t map_size       = 0;
    uint32_t num_cascades   = 0;
    uint32_t num_fft_stages = 0;

    nc::RID spectrum;
    nc::RID displacement_map;
    nc::RID normal_map;

    nc::RID fft_buffer;

    nc::RID cb_spectrum_compute;
    nc::RID cb_spectrum_modulate;
    nc::RID cb_cascade_index;
    nc::RID cb_map_size;
    nc::RID cb_fft_unpack;

    nc::RID set_spectrum_compute;
    nc::RID set_spectrum_modulate;
    nc::RID set_fft_compute;
    nc::RID set_transpose;
    nc::RID set_fft_unpack;

    nc::RID pso_spectrum_compute;
    nc::RID pso_spectrum_modulate;
    nc::RID pso_fft_compute;
    nc::RID pso_transpose;
    nc::RID pso_fft_unpack;

    WaveCascadeParams* pass_cascades = nullptr;
    uint32_t pass_cascade_count      = 0;
    uint32_t pass_remaining          = 0;
};

} // namespace sea
