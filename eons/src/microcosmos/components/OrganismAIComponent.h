#pragma once

#include <string>

#include <ncore/kernel/types.h>

enum class BehaviourState { IDLING = 0, RUN_TUMBLE = 1, ABSORBING = 2, EVALUATE = 3 };

struct OrganismAIComponent {
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

    NSTRUCT(OrganismAIComponent,
            NC_F(OrganismAIComponent, state) NC_F(OrganismAIComponent, move_speed)
                NC_F(OrganismAIComponent, absorb_speed) NC_F(OrganismAIComponent, is_food_found)
                    NC_F(OrganismAIComponent, has_moved) NC_F(OrganismAIComponent, is_absorbing)
                        NC_F(OrganismAIComponent, reproduced) NC_F(OrganismAIComponent, act_interval)
                            NC_F(OrganismAIComponent, moving_interval) NC_F(OrganismAIComponent, act_timer)
                                NC_F(OrganismAIComponent, reproduce_interval))
};
