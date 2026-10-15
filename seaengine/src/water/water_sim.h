#pragma once

namespace nc {
class Node;
}

namespace sea {

class WaveGenerator;

WaveGenerator& get_wave_generator();
nc::Node* register_water_sim( nc::Node* parent );

} // namespace sea
