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
    return 0.076f * std::pow( ( wind_speed * wind_speed ) / ( fetch_meters * GRAVITY ), 0.22f );
}

float WaveGenerator::jonswap_peak_angular_frequency( float wind_speed, float fetch_meters )
{
    return 22.0f * std::pow( ( GRAVITY * GRAVITY ) / ( wind_speed * fetch_meters ), 1.0f / 3.0f );
}

void WaveGenerator::init_gpu(
    RenderService& p_renderer, ResourceLoader& p_resources, uint32_t p_map_size, uint32_t p_num_cascades
)
{
    NC_ASSERT( math::is_power_of_two( p_map_size ) && p_map_size >= 128 && p_map_size <= 1024 );
    NC_ASSERT( p_num_cascades >= 1 );
    NC_ASSERT( p_map_size == DEFAULT_MAP_SIZE );

    if (initialized)
        shutdown( p_renderer );

    map_size       = p_map_size;
    num_cascades   = p_num_cascades;
    num_fft_stages = math::log2_floor( map_size );

    create_resources( p_renderer );
    create_pipelines( p_renderer, p_resources );

    auto* gfx = p_renderer.get_graphics_api();
    gfx->set_queue( GpuQueue::COMPUTE );
    gfx->set_context_state( false );

    gfx->designate_timestamp_slot( WAVE_GEN_TS_SLOT, "Total Compute" );
    gfx->designate_timestamp_slot( WAVE_MODULATE_TS_SLOT, "Wave Modulate" );
    gfx->designate_timestamp_slot( WAVE_FF1_TS_SLOT, "Wave FFT1" );
    gfx->designate_timestamp_slot( WAVE_TRANSPOSE_TS_SLOT, "Wave Transpose" );
    gfx->designate_timestamp_slot( WAVE_FFT2_TS_SLOT, "Wave FFT2" );
    gfx->designate_timestamp_slot( WAVE_UNPACK_TS_SLOT, "Wave Unpack" );

    MapSizeCB map_cb{ map_size, 0, 0, 0 };
    write_cb( p_renderer, cb_map_size, &map_cb, sizeof( map_cb ) );

    initialized = true;
    NC_LOG_INFO(
        "WaveGenerator ready: {}x{}, {} cascades, {} FFT stages", map_size, map_size, num_cascades, num_fft_stages
    );
}

void WaveGenerator::shutdown( RenderService& p_renderer )
{
    if (!initialized)
        return;
    destroy_resources( p_renderer );
    initialized        = false;
    map_size           = 0;
    num_cascades       = 0;
    pass_remaining     = 0;
    pass_cascades      = nullptr;
    pass_cascade_count = 0;
}

void WaveGenerator::update( RenderService& p_renderer, float p_delta, Span<WaveCascadeParams> p_cascades )
{
    NC_ASSERT( initialized );
    NC_ASSERT( !p_cascades.empty() );
    NC_ASSERT( p_cascades.size() <= num_cascades );

    if (pass_remaining != 0)
        flush( p_renderer );

    for (auto& p : p_cascades) {
        p.time += p_delta;
        p.foam_grow_rate  = p_delta * p.foam_amount * 0.5f;
        p.foam_decay_rate = p_delta * std::max( 0.5f, 10.0f - p.foam_amount ) * 0.3f;
    }

    pass_cascades      = p_cascades.data();
    pass_cascade_count = static_cast<uint32_t>( p_cascades.size() );
    pass_remaining     = pass_cascade_count;

    auto* gfx = p_renderer.get_graphics_api();
    gfx->set_queue( GpuQueue::COMPUTE );
    gfx->set_context_state( false );

    gfx->timestamp_begin( WAVE_GEN_TS_SLOT );
    flush( p_renderer );
    gfx->timestamp_end( WAVE_GEN_TS_SLOT );
    ;
}

void WaveGenerator::flush( RenderService& p_renderer )
{
    while (pass_remaining > 0) {
        --pass_remaining;
        Span<WaveCascadeParams> cascades( pass_cascades, pass_cascade_count );
        dispatch_cascade( p_renderer, pass_remaining, cascades );
    }
}

void WaveGenerator::create_resources( RenderService& p_renderer )
{
    auto* gfx = p_renderer.get_graphics_api();

    auto make_array = [&]( const char* name ) -> RID {
        gfx::TextureDesc desc;
        desc.debug_name = name;
        desc.format     = gfx::TextureFormat::RGBA32_FLOAT;
        desc.dimension  = gfx::ResourceDimension::DIM_2D_ARRAY;
        desc.usage      = gfx::ResourceUsage::DEFAULT;
        desc.bind_mask  = gfx::ResourceBindFlags::SHADER_RESOURCE | gfx::ResourceBindFlags::UNORDERED_ACCESS;
        desc.width      = map_size;
        desc.height     = map_size;
        desc.array_size = num_cascades;
        desc.mip_levels = 1;
        return gfx->texture_create( desc );
    };

    spectrum         = make_array( "wave_spectrum" );
    displacement_map = make_array( "wave_displacement" );
    normal_map       = make_array( "wave_normal" );

    const size_t fft_bytes = size_t( num_cascades ) * map_size * map_size * NUM_SPECTRA * 2ull * sizeof( float ) * 2ull;
    fft_buffer             = p_renderer.buffer_create(
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
        return p_renderer.buffer_create(
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

    cb_spectrum_compute  = make_cb( "cb_spectrum_compute", sizeof( SpectrumComputeCB ) );
    cb_spectrum_modulate = make_cb( "cb_spectrum_modulate", sizeof( SpectrumModulateCB ) );
    cb_cascade_index     = make_cb( "cb_cascade_index", sizeof( CascadeIndexCB ) );
    cb_map_size          = make_cb( "cb_map_size", sizeof( MapSizeCB ) );
    cb_fft_unpack        = make_cb( "cb_fft_unpack", sizeof( FftUnpackCB ) );
}

void WaveGenerator::create_pipelines( RenderService& p_renderer, ResourceLoader& p_resources )
{
    auto sh_spectrum  = p_resources.load<Shader>( "shaders/compute/spectrum_compute.slang" );
    auto sh_modulate  = p_resources.load<Shader>( "shaders/compute/spectrum_modulate.slang" );
    auto sh_fft       = p_resources.load<Shader>( "shaders/compute/fft_compute.slang" );
    auto sh_transpose = p_resources.load<Shader>( "shaders/compute/transpose.slang" );
    auto sh_unpack    = p_resources.load<Shader>( "shaders/compute/fft_unpack.slang" );

    {
        gfx::ResourceMappingEntry entries[] = {
            { "spectrum", gfx::ResourceType::TEXTURE_UAV, spectrum },
            { "SpectrumComputeCB", gfx::ResourceType::CONSTANT_BUFFER, cb_spectrum_compute },
        };
        set_spectrum_compute = p_renderer.resource_set_create( *sh_spectrum, 0, entries );
        pso_spectrum_compute =
            p_renderer.compute_pipeline_create( *sh_spectrum, Span<const RID>{ &set_spectrum_compute, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "spectrum", gfx::ResourceType::TEXTURE_UAV, spectrum },
            { "fft_buffer", gfx::ResourceType::BUFFER_UAV, fft_buffer },
            { "SpectrumModulateCB", gfx::ResourceType::CONSTANT_BUFFER, cb_spectrum_modulate },
        };
        set_spectrum_modulate = p_renderer.resource_set_create( *sh_modulate, 0, entries );
        pso_spectrum_modulate =
            p_renderer.compute_pipeline_create( *sh_modulate, Span<const RID>{ &set_spectrum_modulate, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "fft_buffer", gfx::ResourceType::BUFFER_UAV, fft_buffer },
            { "CascadeIndexCB", gfx::ResourceType::CONSTANT_BUFFER, cb_cascade_index },
        };
        set_fft_compute = p_renderer.resource_set_create( *sh_fft, 0, entries );
        pso_fft_compute = p_renderer.compute_pipeline_create( *sh_fft, Span<const RID>{ &set_fft_compute, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "fft_buffer", gfx::ResourceType::BUFFER_UAV, fft_buffer },
            { "CascadeIndexCB", gfx::ResourceType::CONSTANT_BUFFER, cb_cascade_index },
        };
        set_transpose = p_renderer.resource_set_create( *sh_transpose, 0, entries );
        pso_transpose = p_renderer.compute_pipeline_create( *sh_transpose, Span<const RID>{ &set_transpose, 1 } );
    }

    {
        gfx::ResourceMappingEntry entries[] = {
            { "displacement_map", gfx::ResourceType::TEXTURE_UAV, displacement_map },
            { "normal_map", gfx::ResourceType::TEXTURE_UAV, normal_map },
            { "fft_buffer", gfx::ResourceType::BUFFER_UAV, fft_buffer },
            { "FftUnpackCB", gfx::ResourceType::CONSTANT_BUFFER, cb_fft_unpack },
        };
        set_fft_unpack = p_renderer.resource_set_create( *sh_unpack, 0, entries );
        pso_fft_unpack = p_renderer.compute_pipeline_create( *sh_unpack, Span<const RID>{ &set_fft_unpack, 1 } );
    }
}

void WaveGenerator::destroy_resources( RenderService& p_renderer )
{
    auto destroy = [&]( RID& rid ) {
        if (rid) {
            p_renderer.destroy_rid( rid );
            rid = {};
        }
    };

    destroy( pso_spectrum_compute );
    destroy( pso_spectrum_modulate );
    destroy( pso_fft_compute );
    destroy( pso_transpose );
    destroy( pso_fft_unpack );

    destroy( set_spectrum_compute );
    destroy( set_spectrum_modulate );
    destroy( set_fft_compute );
    destroy( set_transpose );
    destroy( set_fft_unpack );

    destroy( cb_spectrum_compute );
    destroy( cb_spectrum_modulate );
    destroy( cb_cascade_index );
    destroy( cb_map_size );
    destroy( cb_fft_unpack );

    destroy( fft_buffer );

    destroy( spectrum );
    destroy( displacement_map );
    destroy( normal_map );
}

void WaveGenerator::write_cb( RenderService& p_renderer, RID buffer, const void* data, size_t size )
{
    p_renderer.buffer_data_write( buffer, data, size );
}

void WaveGenerator::dispatch_cascade(
    RenderService& p_renderer, uint32_t p_cascade_index, Span<WaveCascadeParams> p_cascades
)
{
    auto& params = p_cascades[p_cascade_index];
    auto* gfx    = p_renderer.get_graphics_api();

    gfx->set_queue( GpuQueue::COMPUTE );
    gfx->set_context_state( false );

    if (params.should_generate_spectrum) {
        NC_LOG_DEBUG( "Generating JONSWAP/TMA spectra" );

        const float fetch_m = params.fetch_length_km * 1000.0f;
        SpectrumComputeCB cb{};
        cb.seed_x         = params.spectrum_seed.x;
        cb.seed_y         = params.spectrum_seed.y;
        cb.tile_length_x  = params.scale.tile_length.x;
        cb.tile_length_y  = params.scale.tile_length.y;
        cb.alpha          = jonswap_alpha( params.wind_speed, fetch_m );
        cb.peak_frequency = jonswap_peak_angular_frequency( params.wind_speed, fetch_m );
        cb.wind_speed     = params.wind_speed;
        cb.angle_rad      = params.wind_direction_deg * 0.01745329251f;
        cb.depth          = DEPTH;
        cb.swell          = params.swell;
        cb.detail         = params.detail;
        cb.spread         = params.spread;
        cb.cascade_index  = p_cascade_index;
        write_cb( p_renderer, cb_spectrum_compute, &cb, sizeof( cb ) );

        p_renderer.compute_pipeline_bind( pso_spectrum_compute );
        p_renderer.resource_set_bind( set_spectrum_compute );
        p_renderer.compute_dispatch( map_size / 16, map_size / 16, 1 );

        params.should_generate_spectrum = false;
    }

    {
        SpectrumModulateCB cb{};
        cb.tile_length_x = params.scale.tile_length.x;
        cb.tile_length_y = params.scale.tile_length.y;
        cb.depth         = DEPTH;
        cb.time          = params.time;
        cb.cascade_index = p_cascade_index;
        write_cb( p_renderer, cb_spectrum_modulate, &cb, sizeof( cb ) );

        gfx->timestamp_begin( WAVE_MODULATE_TS_SLOT );
        p_renderer.compute_pipeline_bind( pso_spectrum_modulate );
        p_renderer.resource_set_bind( set_spectrum_modulate );
        p_renderer.compute_dispatch( map_size / 16, map_size / 16, 1 );
        gfx->timestamp_end( WAVE_MODULATE_TS_SLOT );
    }

    {
        CascadeIndexCB cb{ p_cascade_index, 0, 0, 0 };
        write_cb( p_renderer, cb_cascade_index, &cb, sizeof( cb ) );

        gfx->timestamp_begin( WAVE_FF1_TS_SLOT );
        p_renderer.compute_pipeline_bind( pso_fft_compute );
        p_renderer.resource_set_bind( set_fft_compute );
        p_renderer.compute_dispatch( 1, map_size, NUM_SPECTRA );
        gfx->timestamp_end( WAVE_FF1_TS_SLOT );

        gfx->timestamp_begin( WAVE_TRANSPOSE_TS_SLOT );
        p_renderer.compute_pipeline_bind( pso_transpose );
        p_renderer.resource_set_bind( set_transpose );
        p_renderer.compute_dispatch( map_size / 32, map_size / 32, NUM_SPECTRA );
        gfx->timestamp_end( WAVE_TRANSPOSE_TS_SLOT );

        gfx->timestamp_begin( WAVE_FFT2_TS_SLOT );
        p_renderer.compute_pipeline_bind( pso_fft_compute );
        p_renderer.resource_set_bind( set_fft_compute );
        p_renderer.compute_dispatch( 1, map_size, NUM_SPECTRA );
        gfx->timestamp_end( WAVE_FFT2_TS_SLOT );
    }

    {
        FftUnpackCB cb{};
        cb.cascade_index   = p_cascade_index;
        cb.whitecap        = params.whitecap;
        cb.foam_grow_rate  = params.foam_grow_rate;
        cb.foam_decay_rate = params.foam_decay_rate;
        write_cb( p_renderer, cb_fft_unpack, &cb, sizeof( cb ) );

        gfx->timestamp_begin( WAVE_UNPACK_TS_SLOT );
        p_renderer.compute_pipeline_bind( pso_fft_unpack );
        p_renderer.resource_set_bind( set_fft_unpack );
        p_renderer.compute_dispatch( map_size / 16, map_size / 16, 1 );
        gfx->timestamp_end( WAVE_UNPACK_TS_SLOT );
    }
}

} // namespace sea
