#pragma once

#include "EntitySystem.h"
#include "Vector2D.h"

class TransformComponent;
class SpriteComponent;
class RigidBodyComponent;

class Nutrient : public Component {
public:
    explicit Nutrient(const int energy) : curEnergy(energy) {}
    ~Nutrient() override {}

    float curEnergy;

    void OnInit() override;
    void OnUpdate(float delta) override;

    //! \todo Implement proper ID counting
    size_t getID() const { return id; }

    bool caught = false;
    Vector2D organismPos;

private:
    TransformComponent *transform{};
    SpriteComponent *sprite{};
    RigidBodyComponent *rb{};

    size_t id = 0;
};
