#pragma once
#pragma once

#include <Services.h>
#include <System.h>
#include <Viewport.h>

struct SDL_Renderer;

namespace Aeon {

class World;
class Entity;

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

private:
    void RenderSprite(Entity *entityPtr, SDL_Renderer *renderer);
    void RenderTempCircle(Entity *entityPtr, SDL_Renderer *renderer);
    void RenderPhysicsDebug(Entity *entityPtr, SDL_Renderer *renderer);

    Viewport2D *m_viewport = nullptr;
    IGraphicsContext *m_graphics = nullptr;
};

} // namespace Aeon
