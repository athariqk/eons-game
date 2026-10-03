#pragma once

namespace nc {
class Scene;
}

namespace sea {

class WaveGenerator;

WaveGenerator& get_wave_generator();
void register_water_sim( nc::Scene& scene );

} // namespace sea
