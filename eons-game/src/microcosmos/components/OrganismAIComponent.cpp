#include "OrganismAIComponent.h"

std::string OrganismAIComponent::get_current_behavior() const
{
    switch (state) {
        case BehaviourState::IDLING:
        case BehaviourState::RUN_TUMBLE:
            return "Run & Tumble";
        case BehaviourState::ABSORBING:
            return "Absorbing nutrient";
        case BehaviourState::EVALUATE:
            return "Evaluating";
        default:
            return "Unknown";
    }
}
