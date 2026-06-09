#pragma once

#include "Entity.h"
#include "Vector2D.h"

class NutrientComponent;

/**
 * @brief Simple predefined "dumb" AI component for organisms
 * 
 * Neural network could be used later instead
 */
enum class BehaviourState { 
    Idling = 0, 
    RunAndTumble = 1, 
    Absorbing = 2, 
    Evaluate = 3 
};

class OrganismAIComponent : public Aeon::Component {
public:
    OrganismAIComponent(float organismSpeed, float aiInterval) 
        : moveSpeed(organismSpeed), actInterval(aiInterval) {}
    ~OrganismAIComponent() override = default;

    std::string getCurrentBehaviour() const;

    // Movement parameters
    float moveSpeed;
    float absorbSpeed = 0.2f;

    // AI state
    BehaviourState behaviourState = BehaviourState::Idling;

    // Flags
    bool isNutrientFound = false;
    bool hasMoved = false;
    bool isAbsorbing = false;
    bool reproduced = false;

    // Timers
    float actInterval = 10.0f;
    float movingInterval = 10.0f;
    float actTimer = 0.0f;
    float reproduceInterval = 0.0f;

    // References
    NutrientComponent *caughtNutrient = nullptr;
};
