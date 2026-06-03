# AGENTS.md

Guidance for AI coding agents working in this repository.

## First Reads

Read these before making non-trivial changes:

1. [README.md](README.md)
2. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
3. [CMakeLists.txt](CMakeLists.txt)
4. [CMakePresets.json](CMakePresets.json)
5. [CMakeUserPresets.json](CMakeUserPresets.json)
6. [src/main.cpp](src/main.cpp)

## Build And Run (Windows)

Preferred build path is CMake.

1. Configure:

```powershell
cmake --preset default
```

2. Build:

```powershell
cmake --build build --config Debug
```

3. Run:

```powershell
.\build\bin\Debug\eons_d.exe
```

Notes:

- The `default` preset inherits `vcpkg`; ensure `VCPKG_ROOT` is set.
- Release variants are `Release` and `Dist`.
- Assets are copied to the output directory after build by [CMakeLists.txt](CMakeLists.txt).

## Code Boundaries

- Engine/framework code is under [src/engine](src/engine) (namespace `Aeon`).
- Game-specific logic is under [src/game](src/game), especially [src/game/microcosm](src/game/microcosm).
- Keep engine changes generic; avoid introducing game-specific behavior into engine modules.

## Safe Editing Rules

- Do not edit vendored libraries in [vendors](vendors) unless explicitly asked.
- Do not edit generated build outputs in [build](build).
- Prefer CMake over legacy Premake files ([premake5.lua](premake5.lua), [GenerateProject_vs17.bat](GenerateProject_vs17.bat), [GenerateProject_vs16.bat](GenerateProject_vs16.bat)).

## Change Workflow

1. Make minimal, targeted edits.
2. Rebuild affected configuration (`Debug` by default).
3. If behavior changed, run the executable and sanity-check startup and asset loading.

There is no dedicated automated test suite configured at repository root; rely on build success and runtime smoke checks.

## Quick Orientation By Area

- Main loop and lifecycle: [src/engine/core/MainLoop.h](src/engine/core/MainLoop.h)
- Event model: [src/engine/core/Event.h](src/engine/core/Event.h)
- Scene base type: [src/engine/scene/World.h](src/engine/scene/World.h)
- Rendering abstraction: [src/engine/graphics/IGraphicsContext.h](src/engine/graphics/IGraphicsContext.h)
- View transforms: [src/engine/graphics/Viewport.h](src/engine/graphics/Viewport.h)
- Current game scene: [src/game/microcosm/Microcosm.h](src/game/microcosm/Microcosm.h)

## Dependency Notes

Dependencies are managed via vcpkg and in-repo vendor targets:

- SDL3 / SDL3_image
- Box2D
- ImGui
- spdlog
- mINI

See [vcpkg.json](vcpkg.json) and [vendors/CMakeLists.txt](vendors/CMakeLists.txt).