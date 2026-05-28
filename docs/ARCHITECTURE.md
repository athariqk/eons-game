# Architecture Summary

## Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                         MainLoop                             │
│                                                              │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐     │
│  │   Window    │  │   Physics2D  │  │  AudioManager    │     │
│  └─────────────┘  └──────────────┘  └──────────────────┘     │
│         │                                                    │
│         ▼                                                    │
│  ┌─────────────────────┐                                     │
│  │ SDLGraphicsContext  │ (IGraphicsContext)                  │
│  └─────────────────────┘                                     │
│         │                                                    │
│         │ Events (KeyEvent, MouseEvent)                      │
│         ▼                                                    │
└────────────────────────────────────────────────────────────┬─┘
                                                             │
                 ┌───────────────────────────────────────────┘
                 ▼
         ┌───────────────┐
         │  Scene (Base) │  (Abstract, no game logic)
         └───────────────┘
                 │
                 │ provides: IGraphicsContext*, Physics2D, Viewport
                 │ receives: Events
                 │
                 ▼
      ┌──────────────────────┐
      │  MicrocosmScene      │  (Game-specific)
      │                      │ ✅ Game logic here
      │  - Species tracking  │
      │  - Event handling    │
      └──────────────────────┘
                 │
                 ▼
         ┌────────────────┐
         │ EntityManager  │
         │   - Entities   │
         │   - Components │
         └────────────────┘
```

## Event Flow

```
1. User presses 'W' key
         ↓
2. SDL generates SDL_EVENT_KEY_DOWN
         ↓
3. MainLoop::HandleEvents() converts to KeyEvent
         ↓
4. scene->OnEvent(keyEvent)
         ↓
5. MicrocosmScene::OnEvent() tracks key as pressed
         ↓
6. MainLoop::Update() → scene->_OnUpdate()
         ↓
7. Scene::_OnUpdate() updates EntityManager + calls scene->OnUpdate()
         ↓
8. MicrocosmScene::OnUpdate() moves camera based on pressed keys
```

## Render Flow

```
1. MainLoop::Render()
         ↓
2. Clear frame via IGraphicsContext
         ↓
3. scene->_OnRender()
         ↓
4. Scene::_OnRender() calls EntityManager::Draw()
         ↓
5. Component OnDraw() methods render world objects
         ↓
6. scene->OnRender() renders scene-specific overlays/UI (ImGui)
         ↓
7. Present frame
```

## Viewport & Coordinates

- `Viewport2D` stores the SDL window pointer and graphics context.
- Camera defaults to world position `(0,0)` with zoom `1.0`.
- `WorldToScreen` uses:
  - camera-relative translation: `(world - camera)`
  - zoom scaling
  - viewport centering: `+ (width/2, height/2)`
- `ScreenToWorld` is the inverse transformation.
- Gameplay entities are spawned in world space; rendering converts to screen space through `Viewport2D`.

## File Structure

```
src/engine/
├── core/
│   ├── Event.h/cpp              KeyEvent, MouseEvent, BaseEvent
│   ├── MainLoop.h/cpp           Event generation, update/render loop
│   ├── EntitySystem.h           ECS architecture
│   └── ...
├── graphics/
│   ├── IGraphicsContext.h       Graphics abstraction interface
│   ├── SDLGraphicsContext.h     SDL implementation
│   ├── Viewport.h/cpp           Camera + world/screen coordinate transforms
│   └── ...
└── scene/
    └── Scene.h/cpp              Abstract base class + _OnUpdate/_OnRender wrappers

src/game/microcosm/
├── microcosm.h/cpp              Game scene with camera and simulation logic
├── organism.cpp                 Game component
└── ...

docs/
├── ARCHITECTURE_REFACTORING.md  Detailed architectural overview
├── EVENT_SYSTEM.md              Event system deep dive
└── QUICK_REFERENCE.md           Developer quick reference
```

## Future Enhancements

Possible future improvements:

1. **More Event Types**
   - GamepadEvent for controller support
   - TouchEvent for mobile
   - TextInputEvent for text fields

2. **Event Filtering**
   - Scenes can opt out of certain events
   - Event priority system

3. **Complete SDL Abstraction**
   - IWindow interface
   - ITexture interface
   - No SDL types anywhere in public API

4. **Serialization**
   - Record/replay event streams
   - Network synchronization
   - Automated testing via event playback
