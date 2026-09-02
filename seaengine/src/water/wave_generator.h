#pragma once

#include <span>

#include <ncore/core/rid.h>
#include <ncore/services/video/render_service.h>

#include "wave_cascade_params.h"

namespace nc {
class ResourceService;
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
class WaveGenerator {
public:
    static constexpr float kGravity           = 9.81f;
    static constexpr float kDepth             = 20.0f;
    static constexpr uint32_t kNumSpectra     = 4;
    static constexpr uint32_t kMaxCascades    = 8;
    static constexpr uint32_t kDefaultMapSize = 256;

    WaveGenerator() = default;
    ~WaveGenerator();

    WaveGenerator( const WaveGenerator& )            = delete;
    WaveGenerator& operator=( const WaveGenerator& ) = delete;

    void init_gpu( nc::RenderService& renderer, nc::ResourceService& resources, uint32_t map_size,
                   uint32_t num_cascades );

    void shutdown( nc::RenderService& renderer );

    void update( nc::RenderService& renderer, float delta, std::span<WaveCascadeParams> cascades );

    void flush( nc::RenderService& renderer );

    bool is_initialized() const { return initialized_; }
    uint32_t map_size() const { return map_size_; }
    uint32_t num_cascades() const { return num_cascades_; }

    nc::RID displacement_map() const { return displacement_map_; }
    nc::RID normal_map() const { return normal_map_; }

    static float jonswap_alpha( float wind_speed, float fetch_meters );
    static float jonswap_peak_angular_frequency( float wind_speed, float fetch_meters );

private:
    void create_resources_( nc::RenderService& renderer );
    void create_pipelines_( nc::RenderService& renderer, nc::ResourceService& resources );
    void destroy_resources_( nc::RenderService& renderer );

    void dispatch_cascade_( nc::RenderService& renderer, uint32_t cascade_index,
                            std::span<WaveCascadeParams> cascades );

    void write_cb_( nc::RenderService& renderer, nc::RID buffer, const void* data, size_t size );

    bool initialized_        = false;
    uint32_t map_size_       = 0;
    uint32_t num_cascades_   = 0;
    uint32_t num_fft_stages_ = 0;

    nc::RID spectrum_;
    nc::RID displacement_map_;
    nc::RID normal_map_;

    nc::RID butterfly_;
    nc::RID fft_buffer_;

    nc::RID cb_spectrum_compute_;
    nc::RID cb_spectrum_modulate_;
    nc::RID cb_cascade_index_;
    nc::RID cb_map_size_;
    nc::RID cb_fft_unpack_;

    nc::RID set_spectrum_compute_;
    nc::RID set_spectrum_modulate_;
    nc::RID set_fft_butterfly_;
    nc::RID set_fft_compute_;
    nc::RID set_transpose_;
    nc::RID set_fft_unpack_;

    nc::RID pso_spectrum_compute_;
    nc::RID pso_spectrum_modulate_;
    nc::RID pso_fft_butterfly_;
    nc::RID pso_fft_compute_;
    nc::RID pso_transpose_;
    nc::RID pso_fft_unpack_;

    WaveCascadeParams* pass_cascades_ = nullptr;
    uint32_t pass_cascade_count_      = 0;
    uint32_t pass_remaining_          = 0;
};

} // namespace sea
