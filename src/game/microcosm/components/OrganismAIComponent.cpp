#include "OrganismAIComponent.h"

std::string OrganismAIComponent::getCurrentBehaviour() const {
    switch (behaviourState) {
        case BehaviourState::Idling:
        case BehaviourState::RunAndTumble:
            return "Run & Tumble";
        case BehaviourState::Absorbing:
            return "Absorbing nutrient";
        case BehaviourState::Evaluate:
            return "Evaluating";
        default:
            return "Unknown";
    }
}
