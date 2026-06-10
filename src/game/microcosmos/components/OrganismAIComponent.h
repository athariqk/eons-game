#pragma once

#include "Entity.h"
#include "Vec2D.h"

class FoodComponent;

enum class BehaviourState { IDLING = 0, RUN_TUMBLE = 1, ABSORBING = 2, EVALUATE = 3 };

/**
 * @brief Simple predefined "dumb" AI component for organisms
 *
 * Neural network could be used later instead
 */
class OrganismAIComponent : public ncore::Component {
public:
    OrganismAIComponent(float p_speed, float p_think_interval) : move_speed(p_speed), act_interval(p_think_interval) {}
    ~OrganismAIComponent() override = default;

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
    FoodComponent *captured_food = nullptr;

    std::string get_current_behavior() const;
};
