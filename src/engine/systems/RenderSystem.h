#pragma once

#include <Physics.h>
#include <Services.h>
#include <System.h>
#include <Vector2D.h>
#include <Viewport.h>

struct SDL_Renderer;

namespace Aeon {

class World;
class Entity;

/**
 * @brief Context passed to DebugDrawFnc callbacks
 */
struct PhysDebugDrawContext {
    IGraphicsContext *renderer = nullptr;
    Viewport2D *viewport = nullptr;
};

/**
 * @brief Rendering system that draws entities to the screen
 *
 * This system queries entities with renderable components and draws them
 * using the graphics context from the Viewport2D service.
 *
 * Priority: 100 (runs late, after all game logic)
 */
class RenderSystem : public System {
public:
    RenderSystem() {
        SetPriority(100); // Run rendering late
    }

    bool OnInit(World &world) override;
    void OnRender(World &world, IGraphicsContext &graphics) override;
    void OnGuiRender(World &world) override;

private:
    void RenderSprite(Entity *entityPtr);
    void RenderTempCircle(Entity *entityPtr);

    /**
     * TODO: remove this and move ALL physics debug drawing in the Physics class
     */
    void RenderPhysicsDebug(Entity *entityPtr);

    // DebugDrawFnc callbacks (static, receive DebugDrawContext via context ptr)
    static void OnDebugDrawPolygon(const Vector2D *vertices, int vertexCount, uint32_t color, void *context);
    static void OnDebugDrawSolidPolygon(const Vector2D *vertices, int vertexCount, float radius, uint32_t color,
                                        void *context);
    static void OnDebugDrawCircle(Vector2D center, float radius, uint32_t color, void *context);
    static void OnDebugDrawSolidCircle(Vector2D center, float radius, uint32_t color, void *context);
    static void OnDebugDrawSolidCapsule(Vector2D p1, Vector2D p2, float radius, uint32_t color, void *context);
    static void OnDebugDrawSegment(Vector2D p1, Vector2D p2, uint32_t color, void *context);
    static void OnDebugDrawTransform(Vector2D position, float angle, void *context);
    static void OnDebugDrawPoint(Vector2D p, float size, uint32_t color, void *context);

    Viewport2D *m_viewport = nullptr;
    IGraphicsContext *m_graphics = nullptr;
    Physics2D *m_physics = nullptr;

    PhysDebugDrawContext m_debugDrawCtx;
};

} // namespace Aeon
