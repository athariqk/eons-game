#pragma once

#include <modules/physics/physics_service.h>
#include <modules/video/viewport.h>
#include <ncore/runtime/ecs/ecs_system.h>

namespace ncore {

class EcsWorld;
class AssetManager;

struct PhysDebugDrawContext {
    IRenderService *renderer = nullptr;
    Viewport *viewport = nullptr;
};

class EcsRenderSystem : public EcsSystem {
    NCLASS(EcsRenderSystem, EcsSystem)

public:
    EcsRenderSystem() { set_priority(100); }

    void on_init(EcsWorld &world) override;
    void on_render(EcsWorld &world, IRenderService &graphics) override;
    void on_post_update(EcsWorld &world, double delta) override;
    void on_gui_render(EcsWorld &world) override;

private:
    static void OnDebugDrawPolygon(const Vec2 *vertices, int vertexCount, uint32_t color, void *context);
    static void OnDebugDrawSolidPolygon(const Vec2 *vertices, int vertexCount, float radius, uint32_t color,
                                        void *context);
    static void OnDebugDrawCircle(Vec2 center, float radius, uint32_t color, void *context);
    static void OnDebugDrawSolidCircle(Vec2 center, float radius, uint32_t color, void *context);
    static void OnDebugDrawSolidCapsule(Vec2 p1, Vec2 p2, float radius, uint32_t color, void *context);
    static void OnDebugDrawSegment(Vec2 p1, Vec2 p2, uint32_t color, void *context);
    static void OnDebugDrawTransform(Vec2 position, float angle, void *context);
    static void OnDebugDrawPoint(Vec2 p, float size, uint32_t color, void *context);

    IRenderService *renderer = nullptr;
    AssetManager *resources = nullptr;
    IPhysicsService *physics = nullptr;
    Viewport *viewport = nullptr;
    PhysDebugDrawContext debug_draw_ctx;
};

} // namespace ncore
