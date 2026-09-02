#include "water_sim.h"

#include <ncore/application.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/log.h>

#include "wave_generator.h"

namespace sea {

namespace {

WaveGenerator g_waves;
WaveCascadeParams g_cascades[2];
bool g_initialized = false;

} // namespace

void register_water_sim( nc::Scene& scene )
{
    auto* app = scene.get_app_ctx();
    auto* rd  = app->Services.resolve<nc::RenderService>();
    auto* res = app->Services.resolve<nc::ResourceService>();

    // Two cascades with different tile sizes to reduce tiling artifacts.
    g_cascades[0]               = {};
    g_cascades[0].tile_length   = { 50.0f, 50.0f };
    g_cascades[0].spectrum_seed = { 1234, 5678 };
    g_cascades[0].time          = 120.0f;

    g_cascades[1]                    = {};
    g_cascades[1].tile_length        = { 17.0f, 17.0f };
    g_cascades[1].displacement_scale = 0.6f;
    g_cascades[1].normal_scale       = 0.6f;
    g_cascades[1].spectrum_seed      = { 9991, 4242 };
    g_cascades[1].time               = 120.0f + 3.14159265f;

    // map_size must match MAX_MAP_SIZE in fft_compute/transpose/fft_unpack.slang (256).
    g_waves.init_gpu( *rd, *res, WaveGenerator::kDefaultMapSize, 2 );
    g_initialized = true;

    NC_LOG_INFO( "Water sim registered ({}x{}, {} cascades)", g_waves.map_size(), g_waves.map_size(),
                 g_waves.num_cascades() );

    scene.get_ecs()
        .system( "WaveSim" )
        .in( nc::EcsSystemPhase::UPDATE )
        .run( [rd]( nc::EcsIterState& it ) {
            if ( !g_initialized )
                return;
            g_waves.update( *rd, static_cast<float>( it.delta_time() ), g_cascades );
        } );
}

} // namespace sea
