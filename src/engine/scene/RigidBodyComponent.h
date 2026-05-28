#pragma once

#include "EntitySystem.h"

#include "box2d/id.h"

class TransformComponent;
struct Vector2D;

class RigidBodyComponent : public Component {
public:
    void OnInit() override;
    void OnUpdate(float delta) override;
    void OnDraw() override;
    void OnClear() override;

    void ApplyLinearImpulse(const Vector2D &impulse) const;
    void ApplyLinearForce(const Vector2D &force) const;

private:
    TransformComponent *transform = nullptr;

    b2BodyId bodyId = {};
};
