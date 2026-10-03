#include "wave_generator.h"

#include <algorithm>
#include <cmath>

#include <ncore/resources/resource.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/services/video/gfx_interface.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

namespace sea {

using namespace nc;

WaveGenerator::~WaveGenerator() {}

float WaveGenerator::jonswap_alpha( float wind_speed, float fetch_meters )
{
    return 0.076f * std::pow( ( wind_speed * wind_speed ) / ( fetch_meters * kGravity ), 0.22f );
}

float WaveGenerator::jonswap_peak_angular_frequency( float wind_speed, float fetch_meters )
{
    return 22.0f * std::pow( ( kGravity * kGravity ) / ( wind_speed * fetch_meters ), 1.0f / 3.0f );
}

void WaveGenerator::init_gpu(
    RenderService& renderer, ResourceLoader& resources, uint32_t map_size, uint32_t num_cascades
)
{
    NC_ASSERT( math::is_power_of_two( map_size ) && map_size >= 128 && map_size <= 1024 );
    NC_ASSERT( num_cascades >= 1 && num_cascades <= kMaxCascades );
    // FFT shaders currently hardcode MAX_MAP_SIZE = 256.
    NC_ASSERT( map_size == kDefaultMapSize );

    if (initialized_)
        shutdown( renderer );

    map_size_       = map_size;
    num_cascades_   = num_cascades;
    num_fft_stages_ = math::log2_floor( map_size_ );

    create_resources_( renderer );
    create_pipelines_( renderer, resources );

    auto* gfx = renderer.get_graphics_api();
    gfx->set_queue( GpuQueue::COMPUTE );
    gfx->set_context_state( false );

    MapSizeCB map_cb{ map_size_, 0, 0, 0 };
    write_cb_( renderer, cb_map_size_, &map_cb, sizeof( map_cb ) );

    renderer.compute_pipeline_bind( pso_fft_butterfly_ );
    renderer.resource_set_bind( set_fft_butterfly_ );
    renderer.compute_dispatch( ( map_size_ / 2 ) / 64, num_fft_stages_, 1 );
    gfx->sync_queue( GpuQueue::COMPUTE );

    initialized_ = true;
    NC_LOG_INFO(
        "WaveGenerator ready: {}x{}, {} cascades, {} FFT stages", map_size_, map_size_, num_cascades_, num_fft_stages_
    );
}

void WaveGenerator::shutdown( RenderService& renderer )
{
    if (!initialized_)
        return;
    destroy_resources_( renderer );
    initialized_        = false;
    map_size_           = 0;
    num_cascades_       = 0;
    pass_remaining_     = 0;
    pass_cascades_      = nullptr;
    pass_cascade_count_ = 0;
}

void WaveGenerator::update( RenderService& renderer, float delta, Span<WaveCascadeParams> cascades )
{
    NC_ASSERT( initialized_ );
    NC_ASSERT( !cascades.empty() );
    NC_ASSERT( cascades.size() <= num_cascades_ );

    if (pass_remaining_ != 0)
        flush( renderer );

    for (auto& p : cascades) {
        p.time += delta;
        p.foam_grow_rate  = delta * p.foam_amount * 0.5f;
        p.foam_decay_rate = delta * std::max( 0.5f, 10.0f - p.foam_amount ) * 0.3f;
    }

    pass_cascades_      = cascades.data();
    pass_cascade_count_ = static_cast<uint32_t>( cascades.size() );
    pass_remaining_     = pass_cascade_count_;

    auto* gfx = renderer.get_graphics_api();
    gfx->set_queue( GpuQueue::COMPUTE );
    gfx->set_context_state( false );
    gfx->timestamp_begin( GfxInterface::COMPUTE_TIMESTAMP_SLOT );
    flush( renderer );
    gfx->timestamp_end( GfxInterface::COMPUTE_TIMESTAMP_SLOT );
}

void WaveGenerator::flush( RenderService& renderer )
{
    while (pass_remaining_ > 0) {
        --pass_remaining_;
        Span<WaveCascadeParams> cascades( pass_cascades_, pass_cascade_count_ );
        dispatch_cascade_( renderer, pass_remaining_, cascades );
    }
}

void WaveGenerator::create_resources_( RenderService& renderer )
{
    auto* gfx = renderer.get_graphics_api();

    auto make_array = [&]( const char* name ) -> RID {
        gfx::TextureDesc desc;
        desc.debug_name = name;
        desc.format     = gfx::TextureFormat::RGBA32_FLOAT;
        desc.dimension  = gfx::ResourceDimension::DIM_2D;
        desc.usage      = gfx::ResourceUsage::DEFAULT;
        desc.bind_mask  = gfx::ResourceBindFlags::SHADER_RESOURCE | gfx::ResourceBindFlags::UNORDERED_ACCESS;
        desc.width      = map_size_;
        desc.height     = map_size_;
        desc.array_size = num_cascades_;
        desc.mip_levels = 1;
        return gfx->texture_create( desc );
    };

    spectrum_         = make_array( "wave_spectrum" );
    displacement_map_ = make_array( "wave_displacement" );
    normal_map_       = make_array( "wave_normal" );

    const size_t butterfly_bytes = size_t( num_fft_stages_ ) * map_size_ * sizeof( float ) * 4;
    butterfly_                   = renderer.buffer_create(
        gfx::BufferDesc{
            .debug_name      = "wave_butterfly",
            .bind_mask       = gfx::ResourceBindFlags::UNORDERED_ACCESS | gfx::ResourceBindFlags::SHADER_RESOURCE,
            .size            = butterfly_bytes,
            .usage           = gfx::ResourceUsage::DEFAULT,
            .cpu_access_mask = gfx::ResourceCpuAccessFlags::NONE,
            .mode            = gfx::BufferMode::STRUCTURED,
            .element_stride  = sizeof( float ) * 4
        }
    );

    const size_t fft_bytes =
        size_t( num_cascades_ ) * map_size_ * map_size_ * kNumSpectra * 2ull * sizeof( float ) * 2ull;
    fft_buffer_ = renderer.buffer_create(
        gfx::BufferDesc{
            .debug_name      = "wave_fft_buffer",
            .bind_mask       = gfx::ResourceBindFlags::UNORDERED_ACCESS | gfx::ResourceBindFlags::SHADER_RESOURCE,
            .size            = fft_bytes,
            .usage           = gfx::ResourceUsage::DEFAULT,
            .cpu_access_mask = gfx::ResourceCpuAccessFlags::NONE,
            .mode            = gfx::BufferMode::STRUCTURED,
            .element_stride  = sizeof( float ) * 2
        }
    );

    auto make_cb = [&]( const char* name, size_t size ) -> RID {
        return renderer.buffer_create(
            gfx::BufferDesc{
                .debug_name      = name,
                .bind_mask       = gfx::ResourceBindFlags::UNIFORM_BUFFER,
                .size            = size,
                .usage           = gfx::ResourceUsage::DYNAMIC,
                .cpu_access_mask = gfx::ResourceCpuAccessFlags::WRITE,
                .mode            = gfx::BufferMode::NONE
            }
        );
    };

    cb_spectrum_compute_  = make_cb( "cb_spectrum_compute", sizeof( SpectrumComputeCB ) );
    cb_spectrum_modulate_ = make_cb( "cb_spectrum_modulate", sizeof( SpectrumModulateCB ) );
    cb_cascade_index_     = make_cb( "cb_cascade_index", sizeof( CascadeIndexCB ) );
    cb_map_size_          = make_cb( "cb_map_size", sizeof( MapSizeCB ) );
    cb_fft_unpack_        = make_cb( "cb_fft_unpack", sizeof( FftUnpackCB ) );
}

void WaveGenerator::create_pipelines_( RenderService& renderer, ResourceLoader& resources )
{
    auto sh_spectrum  = resources.load<Shader>( "shaders/compute/spectrum_compute.slang" );
    auto sh_modulate  = resources.load<Shader>( "shaders/compute/spectrum_modulate.slang" );
    auto sh_butterfly = resources.load<Shader>( "shaders/compute/fft_butterfly.slang" );
    auto sh_fft       = resources.load<Shader>( "shaders/compute/fft_compute.slang" );
    auto sh_transpose = resources.load<Shader>( "shaders/compute/transpose.slang" );
    auto sh_unpack    = resources.load<Shader>( "shaders/compute/fft_unpack.slang" );

    {
        gfx::ResourceMappingEntry entries[] = {
            { "spectrum", gfx::ResourceType::TEXTURE_UAV, spectrum_ },
            { "SpectrumComputeCB", gfx::ResourceType::CONSTANT_BUFFER, cb_spectrum_compute_ },
        };
        set_spectrum_compute_ = renderer.resource_set_create( *sh_spectrum, 0, entries );
        pso_spectrum_compute_ =
            renderer.compute_pipeline_create( *sh_spectrum, Span<const RID>{ &set_spectrum_compute_, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "spectrum", gfx::ResourceType::TEXTURE_UAV, spectrum_ },
            { "fft_buffer", gfx::ResourceType::BUFFER_UAV, fft_buffer_ },
            { "SpectrumModulateCB", gfx::ResourceType::CONSTANT_BUFFER, cb_spectrum_modulate_ },
        };
        set_spectrum_modulate_ = renderer.resource_set_create( *sh_modulate, 0, entries );
        pso_spectrum_modulate_ =
            renderer.compute_pipeline_create( *sh_modulate, Span<const RID>{ &set_spectrum_modulate_, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "butterfly", gfx::ResourceType::BUFFER_UAV, butterfly_ },
            { "MapSizeCB", gfx::ResourceType::CONSTANT_BUFFER, cb_map_size_ },
        };
        set_fft_butterfly_ = renderer.resource_set_create( *sh_butterfly, 0, entries );
        pso_fft_butterfly_ =
            renderer.compute_pipeline_create( *sh_butterfly, Span<const RID>{ &set_fft_butterfly_, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "butterfly", gfx::ResourceType::BUFFER_SRV, butterfly_ },
            { "fft_buffer", gfx::ResourceType::BUFFER_UAV, fft_buffer_ },
            { "CascadeIndexCB", gfx::ResourceType::CONSTANT_BUFFER, cb_cascade_index_ },
        };
        set_fft_compute_ = renderer.resource_set_create( *sh_fft, 0, entries );
        pso_fft_compute_ = renderer.compute_pipeline_create( *sh_fft, Span<const RID>{ &set_fft_compute_, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "fft_buffer", gfx::ResourceType::BUFFER_UAV, fft_buffer_ },
            { "CascadeIndexCB", gfx::ResourceType::CONSTANT_BUFFER, cb_cascade_index_ },
        };
        set_transpose_ = renderer.resource_set_create( *sh_transpose, 0, entries );
        pso_transpose_ = renderer.compute_pipeline_create( *sh_transpose, Span<const RID>{ &set_transpose_, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "displacement_map", gfx::ResourceType::TEXTURE_UAV, displacement_map_ },
            { "normal_map", gfx::ResourceType::TEXTURE_UAV, normal_map_ },
            { "fft_buffer", gfx::ResourceType::BUFFER_UAV, fft_buffer_ },
            { "FftUnpackCB", gfx::ResourceType::CONSTANT_BUFFER, cb_fft_unpack_ },
        };
        set_fft_unpack_ = renderer.resource_set_create( *sh_unpack, 0, entries );
        pso_fft_unpack_ = renderer.compute_pipeline_create( *sh_unpack, Span<const RID>{ &set_fft_unpack_, 1 } );
    }
}

void WaveGenerator::destroy_resources_( RenderService& renderer )
{
    auto destroy = [&]( RID& rid ) {
        if (rid) {
            renderer.destroy_rid( rid );
            rid = {};
        }
    };

    destroy( pso_spectrum_compute_ );
    destroy( pso_spectrum_modulate_ );
    destroy( pso_fft_butterfly_ );
    destroy( pso_fft_compute_ );
    destroy( pso_transpose_ );
    destroy( pso_fft_unpack_ );

    destroy( set_spectrum_compute_ );
    destroy( set_spectrum_modulate_ );
    destroy( set_fft_butterfly_ );
    destroy( set_fft_compute_ );
    destroy( set_transpose_ );
    destroy( set_fft_unpack_ );

    destroy( cb_spectrum_compute_ );
    destroy( cb_spectrum_modulate_ );
    destroy( cb_cascade_index_ );
    destroy( cb_map_size_ );
    destroy( cb_fft_unpack_ );

    destroy( butterfly_ );
    destroy( fft_buffer_ );

    destroy( spectrum_ );
    destroy( displacement_map_ );
    destroy( normal_map_ );
}

void WaveGenerator::write_cb_( RenderService& renderer, RID buffer, const void* data, size_t size )
{
    renderer.buffer_data_write( buffer, data, size );
}

void WaveGenerator::dispatch_cascade_(
    RenderService& renderer, uint32_t cascade_index, Span<WaveCascadeParams> cascades
)
{
    auto& params = cascades[cascade_index];
    auto* gfx    = renderer.get_graphics_api();

    gfx->set_queue( GpuQueue::COMPUTE );
    gfx->set_context_state( false );

    if (params.should_generate_spectrum) {
        const float fetch_m = params.fetch_length_km * 1000.0f;
        SpectrumComputeCB cb{};
        cb.seed_x         = params.spectrum_seed.x;
        cb.seed_y         = params.spectrum_seed.y;
        cb.tile_length_x  = params.tile_length.x;
        cb.tile_length_y  = params.tile_length.y;
        cb.alpha          = jonswap_alpha( params.wind_speed, fetch_m );
        cb.peak_frequency = jonswap_peak_angular_frequency( params.wind_speed, fetch_m );
        cb.wind_speed     = params.wind_speed;
        cb.angle_rad      = params.wind_direction_deg * 0.01745329251f;
        cb.depth          = kDepth;
        cb.swell          = params.swell;
        cb.detail         = params.detail;
        cb.spread         = params.spread;
        cb.cascade_index  = cascade_index;
        write_cb_( renderer, cb_spectrum_compute_, &cb, sizeof( cb ) );

        renderer.compute_pipeline_bind( pso_spectrum_compute_ );
        renderer.resource_set_bind( set_spectrum_compute_ );
        renderer.compute_dispatch( map_size_ / 16, map_size_ / 16, 1 );

        params.should_generate_spectrum = false;
    }

    {
        SpectrumModulateCB cb{};
        cb.tile_length_x = params.tile_length.x;
        cb.tile_length_y = params.tile_length.y;
        cb.depth         = kDepth;
        cb.time          = params.time;
        cb.cascade_index = cascade_index;
        write_cb_( renderer, cb_spectrum_modulate_, &cb, sizeof( cb ) );

        renderer.compute_pipeline_bind( pso_spectrum_modulate_ );
        renderer.resource_set_bind( set_spectrum_modulate_ );
        renderer.compute_dispatch( map_size_ / 16, map_size_ / 16, 1 );
    }

    {
        CascadeIndexCB cb{ cascade_index, 0, 0, 0 };
        write_cb_( renderer, cb_cascade_index_, &cb, sizeof( cb ) );

        renderer.compute_pipeline_bind( pso_fft_compute_ );
        renderer.resource_set_bind( set_fft_compute_ );
        renderer.compute_dispatch( 1, map_size_, kNumSpectra );

        renderer.compute_pipeline_bind( pso_transpose_ );
        renderer.resource_set_bind( set_transpose_ );
        renderer.compute_dispatch( map_size_ / 32, map_size_ / 32, kNumSpectra );

        renderer.compute_pipeline_bind( pso_fft_compute_ );
        renderer.resource_set_bind( set_fft_compute_ );
        renderer.compute_dispatch( 1, map_size_, kNumSpectra );
    }

    {
        FftUnpackCB cb{};
        cb.cascade_index   = cascade_index;
        cb.whitecap        = params.whitecap;
        cb.foam_grow_rate  = params.foam_grow_rate;
        cb.foam_decay_rate = params.foam_decay_rate;
        write_cb_( renderer, cb_fft_unpack_, &cb, sizeof( cb ) );

        renderer.compute_pipeline_bind( pso_fft_unpack_ );
        renderer.resource_set_bind( set_fft_unpack_ );
        renderer.compute_dispatch( map_size_ / 16, map_size_ / 16, 1 );
    }
}

} // namespace sea
