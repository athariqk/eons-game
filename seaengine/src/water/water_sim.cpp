#include "water_sim.h"

#include <format>

#include <ncore/application.h>
#include <ncore/runtime/components/services.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/config.h>

#include "wave_generator.h"

namespace sea {

static WaveGenerator g_waves;

struct CascadeDefaults {
    float tile;
    float wind;
    float wind_dir;
    float fetch;
    float swell;
    float spread;
    float detail;
    float whitecap;
    float foam;
    float disp;
    float norm;
};

// Reference defaults from 2Retr0/GodotOceanWaves (main.tscn). Each cascade's wind/fetch
// is matched to its tile size so its JONSWAP peak lands in the band the tile resolves.
static constexpr CascadeDefaults kCascadeDefaults[1] = {
    { 88.0f, 10.0f, 20.0f, 150.0f, 0.8f, 0.2f, 1.0f, 0.5f, 8.0f, 1.0f, 1.0f },
    //{ 57.0f, 5.0f, 15.0f, 150.0f, 0.8f, 0.4f, 1.0f, 0.5f, 0.0f, 0.75f, 1.0f },
    //{ 16.0f, 20.0f, 20.0f, 550.0f, 0.8f, 0.4f, 1.0f, 0.25f, 3.0f, 0.0f, 0.25f },
};

// static const nc::Vec2i kCascadeSeeds[1] = { { 1234, 5678 }, { 9991, 4242 }, { 31337, 4242 } };
static const nc::Vec2i kCascadeSeeds[1] = { { 1234, 5678 } };

WaveGenerator& get_wave_generator()
{
    return g_waves;
}

static void load_cascade_config_( const nc::ConfFile& cfg, WaterSimComponent* water_sim )
{
    for (int i = 0; i < water_sim->CascadeCount; ++i) {
        const auto& d         = kCascadeDefaults[i];
        const nc::String base = nc::String( std::format( "Water.Cascade{}", i ) );

        auto& c                    = water_sim->Cascades[i];
        c                          = {};
        const float tile           = cfg.get_float( base + ".TileLength", d.tile );
        c.scale.tile_length        = { tile, tile };
        c.scale.displacement_scale = cfg.get_float( base + ".DisplacementScale", d.disp );
        c.scale.normal_scale       = cfg.get_float( base + ".NormalScale", d.norm );
        c.wind_speed               = cfg.get_float( base + ".WindSpeed", d.wind );
        c.wind_direction_deg       = cfg.get_float( base + ".WindDirection", d.wind_dir );
        c.fetch_length_km          = cfg.get_float( base + ".FetchLength", d.fetch );
        c.swell                    = cfg.get_float( base + ".Swell", d.swell );
        c.spread                   = cfg.get_float( base + ".Spread", d.spread );
        c.detail                   = cfg.get_float( base + ".Detail", d.detail );
        c.whitecap                 = cfg.get_float( base + ".Whitecap", d.whitecap );
        c.foam_amount              = cfg.get_float( base + ".FoamAmount", d.foam );
        c.spectrum_seed            = kCascadeSeeds[i];
        c.time                     = 120.0f + 3.14159265f * static_cast<float>( i );
    }
}

nc::Node* register_water_sim( nc::Node* parent )
{
    auto scene = parent->get_scene();
    auto* app  = scene->get_app_ctx();
    auto* rd   = app->Services.resolve<nc::RenderService>();
    auto& res  = scene->get_resource_loader();

    auto result           = parent->create_child( "WaterSim" );
    auto ws_comp          = result->add_component<WaterSimComponent>();
    ws_comp->CascadeCount = 1;

    auto& app_cfg = scene->get_app_ctx()->Config;
    load_cascade_config_( app_cfg, ws_comp );

    // map_size must match MAX_MAP_SIZE in fft_compute/transpose/fft_unpack.slang.
    g_waves.init_gpu( *rd, res, WaveGenerator::kDefaultMapSize, ws_comp->CascadeCount );

    NC_LOG_INFO(
        "Water sim registered ({}x{}, {} cascades)", g_waves.map_size(), g_waves.map_size(), g_waves.num_cascades()
    );

    scene->get_ecs()
        .observer( "WaterSim_OnSet" )
        .on<WaterSimComponent>( EcsCoreEvent::OnSet )
        .each( []( nc::EcsIterState& it ) {
            auto sim = it.get_component<WaterSimComponent>();
            for (int i = 0; i < sim->CascadeCount; i++) {
                sim->Cascades[i].should_generate_spectrum = true;
            }
        } );

    scene->get_ecs()
        .system( "WaterSim_Updater" )
        .with<WaterSimComponent>()
        .in( nc::EcsSystemPhase::UPDATE )
        .each( []( nc::EcsIterState& it ) {
            auto vid = it.world().get_singleton<nc::VideoServices>();
            auto sim = it.get_component<WaterSimComponent>();
            g_waves.update(
                *vid->Renderer, static_cast<float>( it.delta_time() ),
                { sim->Cascades, static_cast<size_t>( sim->CascadeCount ) }
            );
        } );

    return result;
}

} // namespace sea
