#pragma once

#include <AudioManager.h>
#include <EntitySystem.h>
#include <Event.h>

class IGraphicsContext;
class Physics2D;
class Viewport2D;
struct SDL_Window;

class Scene {
public:
    Scene() { m_entityManager.SetScene(this); }

    void _OnUpdate(double delta, uint64_t ticks);
    void _OnRender();
    void _OnFinish();

public:
    virtual ~Scene() = default;
    virtual void OnInit() = 0;
    virtual void OnEvent(std::shared_ptr<BaseEvent> event) = 0;
    virtual void OnUpdate(double delta, uint64_t ticks) = 0;
    virtual void OnRender() = 0;
    virtual void OnFinish() = 0;

    Entity &CreateEntity();
    std::vector<Entity *> &GetGroup(Group mGroup) { return m_entityManager.GetGroup(mGroup); }

    // System access through interfaces
    Physics2D *GetPhysics2D() const { return m_physics2d; }
    AudioManager *GetAudioManager() const { return m_audio; }
    Viewport2D *GetViewport() const { return m_viewport2d; }

    // Called by engine to set up systems
    void SetPhysics2D(Physics2D *physics) { m_physics2d = physics; }
    void SetAudioManager(AudioManager *audio) { m_audio = audio; }
    void SetViewport(Viewport2D *viewport) { m_viewport2d = viewport; }

protected:
    Physics2D *m_physics2d = nullptr;
    AudioManager *m_audio = nullptr;
    Viewport2D *m_viewport2d = nullptr;

private:
    EntityManager m_entityManager{};
};
