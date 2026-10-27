#pragma once

namespace nc {
class Scene;
class Node;
}

namespace sea {

class WaveGenerator;

WaveGenerator& get_wave_generator();
nc::Node* register_water_sim( nc::Scene* p_scene );

} // namespace sea
