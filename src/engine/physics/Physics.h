#pragma once

#include <Vector2D.h>
#include "box2d/box2d.h"

namespace Aeon {

struct DebugDrawFnc {
    void (*DrawPolygon)(const Vector2D *vertices, int vertexCount, uint32_t color, void *context) = nullptr;
    void (*DrawSolidPolygon)(const Vector2D *vertices, int vertexCount, float radius, uint32_t color,
                             void *context) = nullptr;
    void (*DrawCircle)(Vector2D center, float radius, uint32_t color, void *context) = nullptr;
    void (*DrawSolidCircle)(Vector2D center, float radius, uint32_t color, void *context) = nullptr;
    void (*DrawSolidCapsule)(Vector2D p1, Vector2D p2, float radius, uint32_t color, void *context) = nullptr;
    void (*DrawSegment)(Vector2D p1, Vector2D p2, uint32_t color, void *context) = nullptr;
    void (*DrawTransform)(Vector2D position, float angle, void *context) = nullptr;
    void (*DrawPoint)(Vector2D p, float size, uint32_t color, void *context) = nullptr;

    void *context = nullptr;

    bool drawShapes = true;
    bool drawJoints = true;
    bool drawAABBs = false;
    bool drawMass = false;
    bool drawContacts = false;
};

/**
 * @brief The engine component that handles 2D physics.
 */
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
    void DestroyBody(const b2BodyId &bodyId) const;
    bool IsBodyValid(const b2BodyId &bodyId) const;

    void ApplyLinearImpulse(const b2BodyId &bodyId, const Vector2D &impulse) const;
    void ApplyLinearForce(const b2BodyId &bodyId, const Vector2D &force) const;

	void UpdateDebugDraw();
    DebugDrawFnc &GetDebugDrawFnc();

    /**
     * @brief Sets whether the physics world should provide debug data
     */
    bool isDebugDraw = false;

private:
    b2WorldDef m_worldDef;
    b2WorldId m_worldId{0};
    b2DebugDraw m_b2DebugDraw;
    DebugDrawFnc m_debugDrawFnc;

private:
    float m_timeStep = 1.0f / 60.0f;
    int m_subStepCount = 4;

    void SyncDebugDrawFlags();
};

} // namespace Aeon
