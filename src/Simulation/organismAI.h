#pragma once

#include "Components/EntitySystem.h"
#include "Vector2D.h"

/**
 * Simple predefined "dumb" AI for the organisms
 * neural network could be used later instead
 */
enum BehaviourState { Idling = 0, RunAndTumble = 1, Absorbing = 2, Evaluate = 3 };

class OrganismComponent;
class TransformComponent;
class RigidBodyComponent;
class Nutrient;
class Genes;

class OrganismAI : public Component {
public:
    OrganismAI(float organismSpeed, float aiInterval) : moveSpeed(organismSpeed), actInterval(aiInterval) {}
    ~OrganismAI() override = default;

    void OnInit() override;
    void OnUpdate(float delta) override;

    void runAndTumble();
    void absorbNutrient();
    void checkForNutrients();
    void reproduce(Genes *genes);

    Vector2D getRandomDirection();

    //! \brief Returns behaviour name
    std::string getCurrentBehaviour() const;

    float moveSpeed;
    float absorbSpeed = 0.2f;

    bool runAI = false;

private:
    BehaviourState behaviourState = BehaviourState::Idling;
    OrganismComponent *organism{};
    TransformComponent *transform{};
    RigidBodyComponent *rb{};
    Nutrient *caughtNutrient{};

    float actInterval = 10;
    float movingInterval = 10;
    float actTimer = 0;
    float reproduceInterval = 0;

    bool isNutrientFound = false;
    bool hasMoved = false;
    bool isAbsorbing = false;
    bool reproduced = false;
};
