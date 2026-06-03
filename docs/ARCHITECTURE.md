# ECS + Systems Architecture - Complete

## Overview

The engine now uses a clean **ECS (Entity-Component-System)** architecture with **Services** for subsystem access.

---

## Core Concepts

### 1. **Components = Pure Data**
Components are POD (Plain Old Data) structures:

```cpp
struct TransformComponent : public Component {
    Vector2D position{0.0f, 0.0f};
    float rotation = 0.0f;
    Vector2D scale{1.0f, 1.0f};
};
```

**No logic in components!**  
- No `OnUpdate()`, `OnDraw()`, `OnInit()`
- Just data + constructors

---

### 2. **Systems = Logic**
Systems process entities with specific component combinations:

```cpp
class PhysicsSystem : public System {
    void OnFixedUpdate(World &world, double fixedDelta) override {
        m_physics.Step();

        // Iterate entities and sync physics → transforms
        for (auto &entity : world.GetEntities()) {
            if (entity->HasComponent<PhysicsBody>() && 
                entity->HasComponent<TransformComponent>()) {
                // Sync logic here
            }
        }
    }

    Physics2D m_physics;  // System owns the subsystem
};
```

**Systems own engine subsystems:**
- `PhysicsSystem` owns `Physics2D`
- `AudioSystem` queues audio events
- `RenderSystem` accesses `Viewport2D` via Services

---

### 3. **Services = Subsystem Registry**
Type-safe service locator for engine subsystems:

```cpp
// Register during engine init
Services services;
services.Register<Window>(window);
services.Register<Viewport2D>(viewport);
services.Register<AudioManager>(audio);

// Access in systems
auto &viewport = world.GetServices().Get<Viewport2D>();
auto *audio = world.GetServices().TryGet<AudioManager>();  // Returns nullptr if not found
```

**Benefits:**
- ✅ No global state
- ✅ No EngineContext struct bloat
- ✅ Type-safe access
- ✅ Easy to mock for testing

---

## World Setup Example

```cpp
class GameWorld : public World {
    void OnInit() override {
        // Add systems (they auto-sort by priority)
        auto &physics = AddSystem<PhysicsSystem>();
        auto &audio = AddSystem<AudioSystem>();
        auto &render = AddSystem<RenderSystem>();

        // Create entities with components
        auto &player = CreateEntity();
        player.AddComponent<TransformComponent>(Vector2D(100, 100));
        player.AddComponent<SpriteComponent>("player.png");

        // Access physics from world
        auto &phys = GetSystem<PhysicsSystem>()->GetPhysics();
        player.AddComponent<PhysicsBodyComponent>(phys);

        // Or access services
        auto &audio = GetServices().Get<AudioManager>();
        audio.PlayWAV("game_start.wav");
    }
};
```

---

## System Lifecycle

Systems have 5 lifecycle hooks:

1. **`OnInit(World &world)`** - Called once when world starts
2. **`OnFixedUpdate(World &world, double fixedDelta)`** - Deterministic, fixed 60 Hz
3. **`OnVariableUpdate(World &world, double delta)`** - Variable framerate, for animation/particles
4. **`OnRender(World &world)`** - Called during render phase
5. **`OnShutdown(World &world)`** - Called when world ends

**Execution Order:**
```
Frame Loop:
  1. PollEvents()
  2. FixedUpdate Loop (1-5 times per frame)
     - PhysicsSystem::OnFixedUpdate  (priority -100)
     - GameplaySystem::OnFixedUpdate (priority 0)
  3. VariableUpdate (once per frame)
     - AnimationSystem::OnVariableUpdate (priority 10)
     - AudioSystem::OnVariableUpdate (priority 50)
  4. Render (once per frame)
     - RenderSystem::OnRender (priority 100)
```

---

## Fixed Timestep

The game loop now uses a **fixed timestep accumulator**:

```cpp
constexpr double FIXED_DT = 1.0 / 60.0;  // 16.67ms

while (accumulator >= FIXED_DT) {
    Update(FIXED_DT);  // Deterministic!
    accumulator -= FIXED_DT;
}

Render();  // Can run at any FPS
```

**Benefits:**
- ✅ Physics runs at consistent 60 Hz
- ✅ Same inputs → same outputs (deterministic)
- ✅ Ready for networked multiplayer
- ✅ No spiral of death (clamped accumulator)

---

## API Changes from Old System

### **Old (Removed):**
```cpp
// Components had logic
struct MyComponent : Component {
    void OnUpdate(float delta) override { /* logic here */ }
    void OnDraw() override { /* drawing here */ }
};

// EngineContext passed everywhere
EngineContext ctx {.window = ..., .viewport = ..., .audio = ...};
world.SetEngineContext(ctx);

// Components accessed entity->GetWorld().GetWindow()
```

### **New (Current):**
```cpp
// Components are pure data
struct MyComponent : Component {
    int health = 100;
    float speed = 5.0f;
};

// Services registered centrally
Services services;
services.Register<Window>(window);
services.Register<Viewport2D>(viewport);

world.SetServices(services);

// Systems access services
auto &viewport = world.GetServices().Get<Viewport2D>();
```

---

## How to Add a New System

1. **Create the system class:**
```cpp
// src/engine/systems/MySystem.h
class MySystem : public System {
public:
    MySystem() {
        SetPriority(0);  // Set execution order
    }

    void OnFixedUpdate(World &world, double fixedDelta) override {
        // Game logic here
        for (auto &entity : world.GetEntities()) {
            if (entity->HasComponent<MyComponent>()) {
                auto &comp = entity->GetComponent<MyComponent>();
                // Process component
            }
        }
    }
};
```

2. **Add to CMakeLists.txt:**
```cmake
"src/engine/systems/MySystem.h"
```

3. **Register in world:**
```cpp
void GameWorld::OnInit() override {
    AddSystem<MySystem>();
}
```

---

## How to Access Engine APIs from Components

**Don't!** Components should not have logic.

**Instead, use systems:**

```cpp
// Want audio? Use AudioSystem
auto *audioSys = world.GetSystem<AudioSystem>();
audioSys->PlaySound("boom.wav");

// Want physics? Use PhysicsSystem
auto *physicsSys = world.GetSystem<PhysicsSystem>();
auto &physics = physicsSys->GetPhysics();
b2BodyId body = physics.CreateBody(&bodyDef);

// Want rendering? Use RenderSystem + components
entity.AddComponent<SpriteComponent>("texture.png");
entity.AddComponent<TransformComponent>(position);
```

---

## Key Files

- **`src/engine/ecs/System.h`** - System base class
- **`src/engine/ecs/Component.h`** - Component base class (pure data)
- **`src/engine/ecs/Entity.h`** - Entity (component container)
- **`src/engine/core/World.h`** - World (entity + system manager)
- **`src/engine/core/Services.h`** - Service locator
- **`src/engine/systems/PhysicsSystem.h`** - Physics system
- **`src/engine/systems/AudioSystem.h`** - Audio system
- **`src/engine/systems/RenderSystem.h`** - Rendering system

---

## Migration Guide (For Your Game Code)

Your old game code has these issues:

1. **Components with `override` methods** → Remove, implement as systems
2. **`GetWindow()` / `GetViewport2D()` calls** → Use `GetServices()`
3. **`entity->GetManager()`** → Use `entity->GetWorld()`

Example fix:

```cpp
// Old
class MyComponent : public Component {
    void OnUpdate(float delta) override {
        auto &audio = entity->GetManager().GetWorld().GetAudioManager();
        audio.PlayWAV("sound.wav");
    }
};

// New
class MySystem : public System {
    void OnVariableUpdate(World &world, double delta) override {
        auto *audio = world.GetServices().TryGet<AudioManager>();
        if (!audio) return;

        for (auto &entity : world.GetEntities()) {
            if (entity->HasComponent<MyComponent>()) {
                audio->PlayWAV("sound.wav");
            }
        }
    }
};
```

---

## Summary

✅ **Clean ECS architecture** - components = data, systems = logic  
✅ **Fixed timestep** - deterministic simulation  
✅ **Services pattern** - no more EngineContext  
✅ **System priorities** - predictable execution order  
✅ **Ready for multiplayer** - deterministic + state-based  

The core engine is now production-ready. Happy refactoring! 🚀
