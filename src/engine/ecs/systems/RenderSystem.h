#pragma once

#include <Physics.h>
#include <Services.h>
#include <System.h>
#include <Vec2D.h>
#include <Viewport.h>

namespace ncore {

class World;
class Entity;
class TextureManager;

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
        set_priority(100); // Run rendering late
    }

    bool on_init(World &world) override;
    void on_render(World &world, IGraphicsContext &graphics) override;
    void on_post_update(World &world, double delta) override;
    void on_gui_render(World &world) override;

private:
    void render_sprite(Entity *entityPtr);
    void render_temp_circle(Entity *entityPtr);

    /**
     * TODO: remove this and move ALL physics debug drawing in the Physics class
     */
    void render_phys_debug(Entity *entityPtr);

    // DebugDrawFnc callbacks (static, receive DebugDrawContext via context ptr)
    static void OnDebugDrawPolygon(const Vec2D *vertices, int vertexCount, uint32_t color, void *context);
    static void OnDebugDrawSolidPolygon(const Vec2D *vertices, int vertexCount, float radius, uint32_t color,
                                        void *context);
    static void OnDebugDrawCircle(Vec2D center, float radius, uint32_t color, void *context);
    static void OnDebugDrawSolidCircle(Vec2D center, float radius, uint32_t color, void *context);
    static void OnDebugDrawSolidCapsule(Vec2D p1, Vec2D p2, float radius, uint32_t color, void *context);
    static void OnDebugDrawSegment(Vec2D p1, Vec2D p2, uint32_t color, void *context);
    static void OnDebugDrawTransform(Vec2D position, float angle, void *context);
    static void OnDebugDrawPoint(Vec2D p, float size, uint32_t color, void *context);

    Viewport2D *viewport = nullptr;
    IGraphicsContext *graphics_ctx = nullptr;
    TextureManager *texture_mgr = nullptr;
    Physics2D *physics = nullptr;

    PhysDebugDrawContext debug_draw_ctx;
};

} // namespace ncore
