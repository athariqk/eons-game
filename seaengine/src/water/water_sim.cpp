#include "water_sim.h"

#include <ncore/application.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/resources/resource_loader.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>

#include "wave_generator.h"

namespace sea {

static WaveGenerator g_waves;
static WaveCascadeParams g_cascades[2];

WaveGenerator& get_wave_generator()
{
    return g_waves;
}

void register_water_sim( nc::Scene& scene )
{
    auto* app = scene.get_app_ctx();
    auto* rd  = app->Services.resolve<nc::RenderService>();
    auto& res = scene.get_resource_loader();

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
    g_waves.init_gpu( *rd, res, WaveGenerator::kDefaultMapSize, 2 );

    NC_LOG_INFO(
        "Water sim registered ({}x{}, {} cascades)", g_waves.map_size(), g_waves.map_size(), g_waves.num_cascades()
    );

    scene.get_ecs().system( "WaveSim" ).in( nc::EcsSystemPhase::UPDATE ).run( []( nc::EcsIterState& it ) {
        auto* vid = it.world().get_singleton<nc::VideoServices>();
        g_waves.update( *vid->Renderer, static_cast<float>( it.delta_time() ), g_cascades );
    } );
}

} // namespace sea
