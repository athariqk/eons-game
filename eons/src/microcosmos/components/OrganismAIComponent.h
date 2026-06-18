#pragma once

#include <string>

enum class BehaviourState { IDLING = 0, RUN_TUMBLE = 1, ABSORBING = 2, EVALUATE = 3 };

class OrganismAIComponent {

public:
    OrganismAIComponent() : move_speed(0.0f), act_interval(10.0f) {}
    OrganismAIComponent(float p_speed, float p_think_interval) : move_speed(p_speed), act_interval(p_think_interval) {}

    BehaviourState state = BehaviourState::IDLING;
    float move_speed;
    float absorb_speed = 0.2f;
    bool is_food_found = false;
    bool has_moved = false;
    bool is_absorbing = false;
    bool reproduced = false;
    float act_interval = 10.0f;
    float moving_interval = 10.0f;
    float act_timer = 0.0f;
    float reproduce_interval = 0.0f;

    std::string get_current_behavior() const;
};
