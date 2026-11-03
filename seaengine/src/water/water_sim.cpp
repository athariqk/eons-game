#include "water_sim.h"

#include <format>

#include <ncore/application.h>
#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_events.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/servicedb.h>
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
static constexpr CascadeDefaults CASCADE_DEFAULTS[1] = {
    { 88.0f, 10.0f, 20.0f, 150.0f, 0.8f, 0.2f, 1.0f, 0.5f, 8.0f, 1.0f, 1.0f },
    //{ 57.0f, 5.0f, 15.0f, 150.0f, 0.8f, 0.4f, 1.0f, 0.5f, 0.0f, 0.75f, 1.0f },
    //{ 16.0f, 20.0f, 20.0f, 550.0f, 0.8f, 0.4f, 1.0f, 0.25f, 3.0f, 0.0f, 0.25f },
};

// static const nc::Vec2i CASCADE_SEEDS[1] = { { 1234, 5678 }, { 9991, 4242 }, { 31337, 4242 } };
static const nc::Vec2i CASCADE_SEEDS[1] = { { 1234, 5678 } };

WaveGenerator& get_wave_generator()
{
    return g_waves;
}

static void load_cascade_config_( const nc::ConfFile& cfg, WaterSimComponent* water_sim )
{
    for (int i = 0; i < water_sim->cascade_count; ++i) {
        const auto& d         = CASCADE_DEFAULTS[i];
        const nc::String base = nc::String( std::format( "Water.Cascade{}", i ) );

        auto& c                    = water_sim->cascades[i];
        c                          = {};
        const float tile           = cfg.get_float( base + ".tile_length", d.tile );
        c.scale.tile_length        = { tile, tile };
        c.scale.displacement_scale = cfg.get_float( base + ".displacement_scale", d.disp );
        c.scale.normal_scale       = cfg.get_float( base + ".normal_scale", d.norm );
        c.wind_speed               = cfg.get_float( base + ".wind_speed", d.wind );
        c.wind_direction_deg       = cfg.get_float( base + ".wind_direction", d.wind_dir );
        c.fetch_length_km          = cfg.get_float( base + ".fetch_length", d.fetch );
        c.swell                    = cfg.get_float( base + ".swell", d.swell );
        c.spread                   = cfg.get_float( base + ".spread", d.spread );
        c.detail                   = cfg.get_float( base + ".detail", d.detail );
        c.whitecap                 = cfg.get_float( base + ".whitecap", d.whitecap );
        c.foam_amount              = cfg.get_float( base + ".foam_amount", d.foam );
        c.spectrum_seed            = CASCADE_SEEDS[i];
        c.time                     = 120.0f + 3.14159265f * static_cast<float>( i );
    }
}

/**
 * @brief Mirror the cascade parameters onto the water material's uniforms.
 *
 * Driven by the WaterSimComponent ON_SET observer, so both the initial setup and
 * later editor edits (the inspector calls Node::notify_changed) reach the GPU.
 */
static void apply_cascade_params( const WaterSimComponent& p_sim, nc::Ref<nc::Material> p_material )
{
    // GPU-side layout is float4 per cascade: tile_length.xy, displacement, normal.
    nc::Vec4f scales[WaterSimComponent::MAX_CASCADES] = {};
    for (int i = 0; i < p_sim.cascade_count; ++i) {
        const auto& c = p_sim.cascades[i];
        scales[i]     = nc::Vec4f(
            1.0f / c.scale.tile_length.x, 1.0f / c.scale.tile_length.y, c.scale.displacement_scale,
            c.scale.normal_scale
        );
    }
    p_material->set_param( "numCascades", static_cast<unsigned int>( p_sim.cascade_count ) );
    p_material->set_param( "mapScales", nc::TypedVar::from_array( scales ) );
}

nc::Node* register_water_sim( nc::Scene* p_scene )
{
    auto* app = p_scene->get_app_ctx();
    auto* rd  = app->services.resolve<nc::RenderService>();

    // Registered before the component exists: observers only see events emitted after
    // creation, so a later registration would miss the component's initial ON_SET.
    // Also mirrors cascade parameters onto the material once the scene assigns one.
    p_scene->get_ecs()
        .observer( "WaterSim_OnSet" )
        .on<WaterSimComponent>( nc::ecs::ON_SET )
        .each( []( nc::EcsIterState& it ) {
            auto sim = it.get_component<WaterSimComponent>();
            for (int i = 0; i < sim->cascade_count; i++) {
                sim->cascades[i].should_generate_spectrum = true;
            }

            if (sim->material)
                apply_cascade_params( *sim, sim->material );
        } );

    auto result = p_scene->root()->create_child( "WaterSim" );
    result->add_component<WaterSimComponent>();
    auto ws_cmp           = result->get_component<WaterSimComponent>();
    ws_cmp->cascade_count = 1;

    auto& app_cfg = p_scene->get_app_ctx()->config;
    load_cascade_config_( app_cfg, ws_cmp );

    // map_size must match MAX_MAP_SIZE in fft_compute/transpose/fft_unpack.slang.
    g_waves.init_gpu( *rd, *p_scene, WaveGenerator::DEFAULT_MAP_SIZE, ws_cmp->cascade_count );

    NC_LOG_INFO(
        "Water sim registered ({}x{}, {} cascades)", g_waves.get_map_size(), g_waves.get_map_size(),
        g_waves.get_num_cascades()
    );

    // The component was populated after it was added, so the add-time ON_SET only saw
    // the defaults. Fire it once more now that the cascade configuration is loaded.
    result->notify_changed<WaterSimComponent>();

    p_scene->get_ecs()
        .system( "WaterSim_Updater" )
        .with<WaterSimComponent>()
        .in( nc::EcsSystemPhase::UPDATE )
        .each( []( nc::EcsIterState& it ) {
            auto vid = it.world().get_singleton<nc::VideoServices>();
            auto sim = it.get_component<WaterSimComponent>();
            g_waves.update(
                *vid->renderer, static_cast<float>( it.delta_time() ),
                { sim->cascades, static_cast<size_t>( sim->cascade_count ) }
            );
        } );

    return result;
}

} // namespace sea
