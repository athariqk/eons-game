#pragma once

#include "box2d/box2d.h"

class Physics2D {
public:
    Physics2D();
    ~Physics2D() = default;

    void Step() const;

    float GetTimeStep() const { return m_timeStep; }
    void SetTimeStep(float value) { m_timeStep = value; }

    int GetSubStepCount() const { return m_subStepCount; }
    void SetSubStepCount(int value) { m_subStepCount = value; }

    b2BodyId CreateBody(const b2BodyDef *bodyDef) const;

private:
    b2WorldDef m_worldDef;
    b2WorldId m_worldId{0};

private:
    float m_timeStep = 1.0f / 60.0f;
    int m_subStepCount = 4;
};
