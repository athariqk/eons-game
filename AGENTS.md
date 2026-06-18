# AGENTS.md

Guidance for AI coding agents working in this repository.

## First Reads

Read these before making non-trivial changes:

1. [README.md](README.md)
2. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — see [docs gotchas](#known-doc-discrepancies) below
3. [CMakeLists.txt](CMakeLists.txt)
4. [CMakePresets.json](CMakePresets.json)
5. [src/main.cpp](src/main.cpp)
6. [src/pch.h](src/pch.h) — version macros, AEON_FOR_EACH

## Build And Run (Windows)

Prerequisite: `$env:VCPKG_ROOT` must point to a working vcpkg clone.

Preferred build path is CMake.

1. Configure:

```powershell
cmake --preset windows-debug
```

*(Also available: `windows-release`)*

2. Build:

```powershell
cmake --build build --config Debug
```

3. Run:

```powershell
.\build\bin\Debug\eons_d.exe
```
*(Debug builds append `_d` per `CMakeLists.txt:116`)*

Notes:

- Release variants: `cmake --build build --config Release` (binary `eons.exe`), `--config Dist`.
- Assets and `.ini` files are auto-copied to the output directory at build time by `cmake/copy_files.cmake`.

## Code Boundaries

- Engine/framework code is under [src/engine](src/engine) (namespace `Aeon`).
- Game-specific logic is under [src/game](src/game), especially [src/game/microcosm](src/game/microcosm).
- Keep engine changes generic; avoid introducing game-specific behavior into engine modules.

## Safe Editing Rules

- Do not edit vendored libraries in [vendors](vendors) unless explicitly asked.
- Do not edit generated build outputs in [build](build).
- Do NOT delete or rebuild the `build/` directory. The existing build folder is the canonical build; reconfiguring from scratch is slow and unnecessary.
- Prefer CMake over legacy Premake files ([premake5.lua](premake5.lua), [GenerateProject_vs17.bat](GenerateProject_vs17.bat), [GenerateProject_vs16.bat](GenerateProject_vs16.bat)).

## Change Workflow

1. Make minimal, targeted edits.
2. Wait for the user to explicitly request a build. **Do not rebuild automatically after every edit.**
3. If behavior changed, run the executable and sanity-check startup and asset loading.

There is no dedicated automated test suite configured at repository root; rely on build success and runtime smoke checks.

## Quick Orientation By Area

- Main loop and lifecycle: [src/engine/MainLoop.h](src/engine/MainLoop.h)
- Event model: [src/engine/Event.h](src/engine/Event.h)
- Scene base type: [src/engine/World.h](src/engine/World.h)
- Rendering abstraction: [src/engine/graphics/IRenderContext.h](src/engine/graphics/IRenderContext.h)
- View transforms: [src/engine/graphics/Viewport.h](src/engine/graphics/Viewport.h)
- Current game scene: [src/game/microcosm/MicrocosmWorld.h](src/game/microcosm/MicrocosmWorld.h)

## Known Doc Discrepancies

`docs/ARCHITECTURE.md` contains stale claims that will cause compile errors if followed literally:

- **`world.GetServices()` does not exist.** The correct access is `world.GetMainLoop().GetServices()`.
- **`OnRender` signature** is `OnRender(World &world, IRenderContext &graphics)`, not just `(World &world)`.
- **Actual system hooks** (in `src/engine/ecs/System.h`): `OnInit`, `OnFixedUpdate`, `OnVariableUpdate`, `OnPostUpdate`, `OnRender`, `OnGuiRender`, `OnShutdown`. The doc lists 5 of 7, missing `OnPostUpdate` and `OnGuiRender`.
- **Camera class** is `Camera2D`, not `Camera`.

## Dependency Notes

Dependencies are managed via vcpkg and in-repo vendor targets:

- SDL3 / SDL3_image
- Box2D
- ImGui
- spdlog
- mINI

See [vcpkg.json](vcpkg.json) and [vendors/CMakeLists.txt](vendors/CMakeLists.txt).