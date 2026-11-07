# SeaEngine Vision — Implications for the Editor

This document extracts key implications from [seaengine-vision.md](./seaengine-vision.md) and maps them to the editor's philosophy, features, and architecture.

The editor is not a neutral technical tool. For SeaEngine, it is a creative sanctuary where the same sense of wonder, awe, and inspiration must be preservable during creation.

**Current Architecture (as of late 2026):**

- SeaEngine is a **single executable**.
- Editor code lives as a module under `seaengine/editor/` (not a separate application).
- Default launch is an immersive, SpaceEngine-like experience.
- Editor mode is activated via `--editor` command-line flag (or in-app menu / toggle planned; currently F12 in Explore prints the hint).
- Editor-related systems are **disabled** when editor mode is off.
- Editor code is **always compiled in** (not Debug-only) but remains inactive by default.

**Migration status:** Old `ncore_editor` shared library + `register_editor_plugin` model retired. The previous separate `seaengine-editor` executable direction was superseded in favor of a single unified "SeaEngine" binary.

`tools/editor/` (old location) is archived/deprecated. `eons-game` no longer references the editor plugin. See root `AGENTS.md` and `seaengine/editor/`.

## Current Editor Mode Model

SeaEngine uses a mode-based approach for the editor:

- **Explore Mode** (default): Immersive, SpaceEngine-like experience. Editor UI and editor-related systems are disabled.
- **Editor mode** can be activated in two ways:
  1. Command line: `SeaEngine --editor`
  2. In-app: A menu option to enable editor capability at runtime.
- Editor-related systems are **disabled** when not in editor mode.
- The editor code is **always compiled into the binary** (not guarded behind `DEBUG`). It is simply not active by default.
- When switched to Explore Mode, all editor systems, UI, and related processing are turned off to keep the experience clean and performant.

This model prioritizes a high-quality default immersive experience while still making powerful authoring tools easily accessible.

Confirmed decisions:
- Default mode is named **Explore Mode**.
- Only `--editor` on the command line (plus an in-app menu to activate editor capability).
- Editor-related systems are disabled when switched to Explore Mode.
- Editor code is always compiled in (not Debug-only) and not active by default.

---

## 1. The Editor Must Support "Feeling the World"

**Vision principle:**  
Some experiences (Walter Mitty, SpaceEngine, Subnautica) leave a lasting impression. The final software should be able to move people emotionally.

**Implication for the editor:**

- **Runtime Preview is not a secondary feature.** It is central to the workflow.
- The editor must allow the creator to quickly drop into something that feels close to the final experience — with minimal UI chrome, full visual fidelity, and contextual atmosphere.
- There should be "feeling modes" or "explorer modes" where the author can travel through the world the way a player would, and still be able to make adjustments without breaking immersion.
- The gap between "editing" and "being moved by the ocean" should be intentionally small.

**Current state tension:**  
The current editor is heavily inspector + gizmo oriented. The existing "Game View" + planned RuntimePreview work is a good start, but we need to treat high-fidelity, low-distraction preview as a primary use case rather than "the game view with fewer overlays."

---

## 2. Atmosphere (Music, Ambient, SFX) is a First-Class Authoring Domain

**Vision principle:**  
The contextual music and sound design in SpaceEngine was called out as one of the most powerful and underrated aspects. The same applies to diving through the mesopelagic or approaching hydrothermal vents.

**Implication for the editor:**

- Audio/atmosphere cannot be treated as an afterthought system that only exists at runtime.
- The editor needs strong, first-class support for authoring **context-sensitive atmosphere**:
  - Depth-based, biome-based, time-of-day, weather, or event-driven layers.
  - Visual authoring of atmosphere zones or triggers.
  - Ability to audition music + ambient while moving through the world.
- "What does this place sound like when I'm traveling at whale speed?" should be answerable inside the editor.

This is likely to require new panel types or major extensions to existing ones (beyond the current Console/Assets/Inspector/SceneTree set).

---

## 3. Multi-Scale, Seamless, "Infinite" Worlds

**Vision principle:**  
The technological and artistic heart is seamless ocean exploration — from the surface twilight to the abyssal plain without loading screens or hard boundaries.

**Implication for the editor:**

- The editor cannot assume a small, fully-loaded bounded scene.
- Authoring must support working at radically different scales:
  - Macro (regional ocean currents, large biomes)
  - Meso (reefs, trenches, seamounts)
  - Micro (specific vents, artistic details)
- Navigation, selection, and editing tools must remain usable when the world is vast.
- Concepts like streaming, level-of-detail, and procedural + hand-authored hybrids must be considered in the editor's world model (not just the runtime).

This has architectural weight for how the editor owns or manages `Scene` / world instances.

---

## 4. Artistic + Realistic + Majestic Hybrid

**Vision principle:**  
The world should feel realistic *and* artistic/majestic at the same time (twilight sea with birds, light shafts, etc.). "The sublime" is valued.

**Implication for the editor:**

- We need more than raw material + transform editing.
- There should be support for **artistic intent layers**:
  - Per-region or per-shot mood/lighting overrides.
  - Artistic composition aids (framing, emphasis, "this should feel grand").
  - Easy ways to author heightened moments without fighting the underlying simulation.
- The editor should help the author compose beauty, not just place objects.

---

## 5. Exploration as Joy (Even While Authoring)

**Vision principle:**  
Free exploration and the feeling of discovery are core to the experience.

**Implication for the editor:**

- The editor camera and navigation experience matters a lot.
- Current free-cam in the viewport is a starting point, but we may want:
  - Better "explorer" controls tuned for ocean travel.
  - Quick travel / bookmarking of emotionally important locations.
  - Multiple simultaneous viewports bound to different contexts or scales.
- Authoring should feel like *exploring and shaping* rather than "working in a level editor."

---

## 6. Experience Authoring, Not Just Scene Authoring

**Vision principle:**  
Later we will add story, a blue whale protagonist, grounded mystery, and environmental storytelling ("weird object on the beach").

**Implication for the editor:**

- Raw ECS components and property grids are necessary but not sufficient.
- We will eventually need higher-level authoring concepts:
  - Discovery moments
  - Emotional beats / "this should feel wondrous or mysterious"
  - Narrative triggers and environmental storytelling tools
  - Creature-perspective considerations (blue whale scale, senses, movement)
- The editor should help compose *experiences*, not only place entities.

This aligns with (and expands) the direction in the simulation modes spec.

---

## 7. The Editor Itself Should Not Kill the Magic

**Vision principle:**  
The goal is inspiration and quiet awe. The tool used to build it should not feel bureaucratic or soul-crushing.

**Implication for the editor:**

- UI density, performance, and workflow friction directly affect whether the vision can be realized.
- "Runtime Preview" and low-chrome modes are partly about preserving the creator's emotional state.
- The editor should feel like a place where wonder can be cultivated, not just data managed.
- This influences decisions about:
  - How much the editor UI can be hidden or themed.
  - Performance of the editor when the ocean simulation is running.
  - How "always live" the world feels while editing.

---

## 8. Sandbox First, Directed Experience Second

**Vision principle:**  
Current focus is the strong sandbox exploration experience. Story and blue whale gameplay come later on top of that foundation.

**Implication for the editor:**

- Early editor features should prioritize:
  - World building at scale
  - Free movement and presence
  - Atmospheric and visual fidelity
  - Live editing while the ocean "feels alive"
- Features that are primarily for directed story/gameplay (quest systems, dialogue, specific creature mechanics) can be deprioritized until the sandbox is solid.

---

## Impact of the Unified Executable Model

SeaEngine uses a **single executable** model. The editor is integrated into the main `SeaEngine` binary rather than being a separate application.

Key characteristics (confirmed):

- Default experience is immersive (SpaceEngine-like).
- Editor mode is activated via `--editor` or an in-app menu.
- Editor systems are disabled when editor mode is off.
- Editor code is always compiled in (not Debug-only) but inactive by default.

This is cleaner for branding ("SeaEngine") and deployment, while still allowing the editor to feel like a distinct creative tool when activated. The previous "dedicated `seaengine-editor` executable" direction was superseded by this unified approach.

### Alignment with Existing Work

The `.scratch/editor-simulation-modes/spec.md` (Edit / Simulate / RuntimePreview + snapshot/revert) is already well-aligned with the vision. The "feeling" and "live editing during simulation" requirements in that spec are even more important in light of SeaEngine's goals.

---

## Concrete Recommendations for the Migration Architecture

**User decisions (confirmed):**

1. The editor code is **completely merged/absorbed** into `seaengine/editor/` as a module (no separate executable).
2. Single executable named "SeaEngine" (CMake target: `seaengine`).
3. Default mode is named **Explore Mode**.
4. Only `--editor` command line (plus in-app menu).
5. Editor systems disabled in Explore Mode.
6. Editor code always compiled in, not active by default.
7. Old plugin model dropped.
8. `tools/editor/` has been removed (historical references only remain in .scratch/ specs).

This aligns strongly with the SeaEngine vision. The editor is now a first-class part of the SeaEngine project rather than general engine tooling.

### 1. Target Structure (Current)

- Single executable (CMake target: `seaengine`, app name "SeaEngine")
- Editor code lives under `seaengine/editor/` as an internal module (included via `add_subdirectory`)
- No separate editor executable

**Consequences:**
- One binary for both Explore Mode and authoring
- Editor is activated at runtime via `--editor` or in-app menu
- Simpler build, asset staging, and user experience story

### 2. Drop the Plugin Model Completely

- `register_editor_plugin()` / `unregister_editor_plugin()` deleted (old public API; history only).
- Editor initialization now lives inside the single `SeaEngine` executable.
- Editor owns its systems and state via `bootstrap_editor()` / `shutdown_editor()`, but these are only active when editor mode is enabled.

This removes the old "injected plugin" model entirely. Editor capability is now an internal mode of the main application.

### 3. Editor Location and Ownership

The editor code lives under the SeaEngine project as a module (not a separate executable):

```
seaengine/
├── editor/                  # Editor systems, panels, EditorWorldHost, etc.
│   ├── CMakeLists.txt
│   └── src/
├── main.cpp                 # Single SeaEngine entry point (decides mode)
└── src/
```

Editor capability is activated at runtime (via `--editor` or in-app menu). Editor sources are listed directly as plain files inside `seaengine/CMakeLists.txt` (no separate `add_subdirectory(editor)` or editor CMakeLists.txt). Vendored dependencies live under `seaengine/external/`.

### 4. World Ownership Model (Simplified by Decisions)

Because there is no longer a "game Scene" to inject into, we can design cleanly:

- Introduce `EditorWorldHost` (or similar) early.
- It owns:
  - The primary authoring `Scene`
  - One or more `SimulationContext`s
  - Preview mode state
- `Editor` (the existing class) can stay as the high-level UI shell.
- Panels continue to receive stable dependencies + per-frame `EditorFrame`.

This is now much easier because the editor fully controls the world(s).

### 5. Bootstrap Flow (Historical / Superseded)

Rough initialization order inside what was planned as a dedicated editor executable
(current reality: single seaengine binary + EditorWorldHost when --editor):

1. `main()` creates `EditorApplication`.
2. `EditorApplication::init()` → normal engine init + services.
3. Create primary authoring world (`Scene` or wrapped).
4. Inside the world setup:
   - Register editor components (no longer gated behind plugin).
   - Register GUI plugin.
   - Create `Editor`.
   - Spawn editor systems (dockspace, UI, canvas pass, etc.).
   - Spawn EditorCamera.
5. Normal game loop runs with editor systems mixed in as before.
6. On shutdown: clean shutdown of editor-owned state before services.

The existing `create_editor(scene)` and `editor_factory` logic can be reused with almost no change.

### 6. Cleanup Required in Other Projects

- `eons-game/`:
  - Remove `#include <editor/ncore_editor.h>`
  - Remove `register_editor_plugin` / `unregister...` calls.
  - Remove `ncore_editor` from `target_link_libraries`.
- `seaengine/`:
  - Same as above (in `world_scene.cpp` and its CMake).
- Root `CMakeLists.txt`:
  - Update install rules (now installing an executable instead of a library).
  - Update asset pipeline dependencies if needed.
  - Update documentation / build commands.

### 7. Build System Changes

In `tools/editor/CMakeLists.txt`:

```cmake
# Before
add_library(ncore_editor SHARED)

# After
add_executable(SeaEngineEditor)

# Adjust sources (remove the old public header)
# Keep most sources the same
```

- Rename the target or set `OUTPUT_NAME` to get a nice binary (`SeaEngineEditor_d.exe`).
- Move the public header out of the build or delete it.
- The reflection generation step should still work.

### 8. Phased Implementation (Under New Decisions)

| Phase | Work |
|-------|------|
| 1     | Change CMake to `add_executable`. Create minimal `main.cpp` that boots an `Application` + Scene and wires the editor systems directly. |
| 2     | Done: Refactored (now `editor_bootstrap.cpp`); registration logic internal to seaengine/editor (bootstrap_editor). |
| 3     | Remove all editor includes, linking, and calls from `eons-game` and `seaengine`. |
| 4     | Introduce `EditorApplication` + `EditorWorldHost` skeleton. |
| 5     | Clean up old public API (`include/editor/`, export macros, etc.). |
| 6     | Start adding vision-specific improvements (better preview modes, atmosphere surface, etc.). |

### 9. Benefits for SeaEngine Vision

This direction is excellent for the vision:

- The editor can be designed from day one as the primary creative tool.
- Much easier to implement strong "Explorer" and "Runtime Preview" modes with minimal UI.
- No mental overhead of "is the editor injected or not?"
- Full control over world ownership enables the multi-context / feeling-the-ocean goals.
- Cleaner separation between "developing SeaEngine" and "running SeaEngine".

### 10. Risks / Things to Watch

- The editor must now fully own window + render initialization for its own use (it already did most of this indirectly).
- Make sure the editor still gets the same asset search paths and compiled assets as before.
- ImGui context ownership and shutdown order must be handled carefully (already delicate).
- If other tools or prototypes ever wanted to embed the editor, they will need a different approach in the future (we are accepting this trade-off).

### 1. Target Structure

**Recommended outcome:**

- A first-class standalone executable: `SeaEngineEditor` (Debug: `SeaEngineEditor_d.exe`).
- The editor becomes the primary way to develop and experience SeaEngine during creation.
- `ncore_editor` remains a shared library for now (for panels, assets, inspector, etc.), but the **host logic** moves into the standalone exe.
- Eventually consider splitting `ncore_editor` into:
  - `ncore_editor_core` (static or shared) — reusable panels, assets, property system.
  - Thin host executable.

**Why this fits the vision:**
A dedicated executable makes it natural to design "explorer modes" and high-fidelity previews without carrying game executable baggage.

### 2. World & Simulation Ownership Model (Most Important Change)

**Current problem:** The editor is a guest that injects systems into a `Scene` owned by the game.

**Recommended new model:**

Introduce an `EditorWorldHost` (or `SimulationOrchestrator`) owned by the editor (originally envisioned as standalone exe; implemented inside unified `seaengine`):

- The editor owns **one or more World Instances**.
- Distinguish clearly:
  - **Authoring Instance**: Represents the stable, savable authored state. This is what the scene tree and most inspectors write to.
  - **Simulation Contexts**: One or more derived/live instances that can run at different time scales, with different preview settings, or as "what-if" branches.
- The `SimulationController` (from the existing spec) should live at the `EditorWorldHost` level, not just inside one Scene.

**Benefits for SeaEngine:**
- Enables multiple simultaneous previews (different depths, times of day, or moods).
- Makes snapshot/revert for "Play" much cleaner.
- Supports the "always feels alive" sandbox requirement.

**Migration path (largely complete):**
- Phase 1: Done — standalone exe creates `Scene`, calls internal `bootstrap_editor()` (no more public register).
- Phase 2/3: Ongoing — editor owns init; future work for multi-world / EditorWorldHost as needed (see editor-simulation-modes spec).

### 3. Evolution of the Plugin Registration Model

Do **not** keep the current pattern as the long-term architecture for SeaEngine.

**Recommendation:**

- Keep `register_editor_plugin(Scene&)` temporarily for compatibility and for `eons-game`.
- For the standalone SeaEngine editor, move toward **internal editor-driven registration**.
- Over time, make the public plugin API internal or mark it as "for advanced embedding only".
- The editor should be responsible for registering its own systems, components, and simulation gates rather than relying on the game Scene to opt-in.

This reduces pollution in `seaengine/` code and makes the editor feel like the primary host.

### 4. First-Class Preview & Feeling Modes

**Strong recommendation:** Treat "Explorer Mode" / high-fidelity preview as a core architectural concern from the start of the migration.

Proposed design:

- Add a `PreviewMode` concept (or extend `EditMode`):
  - `Authoring` (full UI, gizmos, inspectors)
  - `Explorer` (minimal UI, full visuals + atmosphere, free movement)
  - `Simulation` (as defined in the modes spec)
  - `RuntimePreview` (closest to final player experience)
- Viewports can be bound to a specific `PreviewContext`.
- The editor should support a "Focus Viewport" command that can temporarily hide most panels and maximize one viewport with reduced editor systems active.

This directly supports the vision goal of being able to "feel the world" while still working.

### 5. Atmosphere & Contextual Experience Authoring

Even if the full runtime atmosphere system doesn't exist yet, the editor architecture should anticipate it.

**Recommendations:**

- Design an extension point early (e.g., `AtmosphereAuthoring` subsystem or editor-only data structures for zones/triggers).
- Plan for a dedicated **Atmosphere Panel** (or major extension of existing panels) that can:
  - Define context rules (depth, biome, time, player state).
  - Preview audio layers while moving.
- Make `EditorFrame` (the per-frame data passed to panels) carry atmosphere context information so panels can react.

Start small: even a simple zone visualization + inspector extension is better than nothing.

### 6. Multi-Scale Authoring Support

**Recommendation:**

Do not wait until the runtime streaming is perfect. Introduce early concepts that help with scale:

- Hierarchical zone / region system (even if lightweight at first).
- Different "editing contexts" or "focus levels" (Ocean Overview vs Local Detail).
- Improved bookmarking and quick navigation between distant locations.
- Consider whether the main viewport should support a "world map" overlay or secondary navigation view for large oceans.

### 7. Decoupling & Game Content Strategy

For SeaEngine specifically:

- (Historical note) The editor (now a compiled-in module) loads components etc. from the unified `seaengine` binary when --editor. No separate loading path was ultimately needed.
- Move toward a model where SeaEngine can provide a "module" (or just rely on reflection) that the editor can consume.
- (Historical) Editor systems were never registered into WorldScene; Explore Mode deliberately has zero editor UI/systems. Editor code lives only under seaengine/editor/ and is activated exclusively via EditorWorldHost in the unified binary.

### 8. Simulation Modes Integration

**Recommendation:**

Make the work described in `.scratch/editor-simulation-modes/spec.md` the foundation of the new editor's world management rather than an add-on.

Specifically:

- The `SimulationController` should be a core object owned by `EditorWorldHost`.
- Support running multiple simulation contexts simultaneously (very powerful for ocean condition testing).
- Ensure live editing (inspector, gizmos, assets) works cleanly in Simulate mode — this is non-negotiable for the vision.

### 9. Build & Project Layout Recommendations

- Editor sources are listed directly inside `seaengine/CMakeLists.txt` (the old `seaengine/editor/CMakeLists.txt` has been merged in and removed).
- No `add_subdirectory(editor)`.
- Vendored externals are under `seaengine/external/` (add_subdirectory(external) from seaengine/CMakeLists.txt).
- Main target: `seaengine` (plain sources from editor/ + src/)
- Binary: `seaengine` (with `_d` in Debug)
- Uses the single unified `seaengine.ini` (both modes share config + log file)
- Asset pipeline stages for the single `seaengine` target
- Include dir for editor code: `seaengine/editor` (flattened; no src/ subdir)

### 10. Phased Migration Plan (Suggested)

| Phase | Goal | Key Changes |
|-------|------|-------------|
| 1 | Move editor under seaengine/ | Create `seaengine/editor/`, copy code, set up new CMake target |
| 2 | Drop plugin model | Remove `register_editor_plugin` calls, bootstrap editor internally |
| 3 | Clean up consumers | Remove editor linking/registration from SeaEngine runtime and eons-game |
| 4 | Own world & preview | Introduce `EditorWorldHost`, strong Explorer/RuntimePreview modes |
| 5 | Add vision-aligned features | Atmosphere authoring, multi-context support, scale tools |
| 6 | (Later) Higher-level tools | Discovery moments, emotional beats, creature perspective |

---

## Migration Complete (Nov 2026)

**Status:** Migration from the old `ncore_editor` shared library + plugin model to the current architecture is complete.

Current model: Single `seaengine` executable (app name "SeaEngine") with editor as an internal module (`seaengine/editor/`). Default is **Explore Mode**. Editor mode is opt-in via `--editor` or in-app menu. Editor systems are disabled in Explore Mode. Editor code is always compiled in.

### Key Changes Delivered
- Old `ncore_editor` shared library + `register_editor_plugin` model fully retired.
- Editor code moved under `seaengine/editor/` as a module (not a separate executable).
- Single `seaengine` executable (app name "SeaEngine") adopted.
- Default mode named **Explore Mode**.
- Editor mode activated via `--editor` flag or in-app menu.
- Editor systems disabled in Explore Mode.
- Editor code always compiled in (not Debug-only) and not active by default.
- Vendored dependencies (ImGui, ImGuizmo, FreeType) migrated as plain trees under `seaengine/external/` (moved out of editor/ in follow-up structural cleanup; editor/ sources flattened with no `src/` subdir).
- `tools/editor/` (old top-level location) fully removed (git rm + deinit).
- `eons-game` and main runtime no longer have editor plugin dependencies.

### Submodule Handling (Chosen Approach)
- **Copy the trees cleanly** into `seaengine/editor/external/` (removing `.git` files/dirs).
- Rationale: ImGui-family libs are typically vendored; avoids ongoing submodule management friction, path changes, and init steps for contributors. Updates can be done via manual tree replacement if needed.
- (Alternative considered: re-registering as direct submodules under `seaengine/editor/external/{imgui,imguizmo,freetype}` — rejected for simplicity.)

### Removal of `tools/editor/` (Completed)
The removal steps (documented in root `AGENTS.md`) were executed:

- `git rm -r tools/editor` + submodule deinit performed.
- Stale `.git/modules/tools/editor` cleaned (as needed).
- Verified build + smoke: Explore Mode and `--editor` both functional.
- No remaining references to old paths in active docs / code / CMake.

### Post-Migration Verification (Done)
- Build: `ninja -C build/windows-debug seaengine` succeeds.
- Explore Mode: `./build/windows-debug/bin/Debug/seaengine` — clean immersive experience.
- Editor mode: `./build/windows-debug/bin/Debug/seaengine --editor` — full UI/panels.
- `seaengine/external/` contains clean vendored trees (no .git). `seaengine/editor/` contains only editor sources (flattened).
- All docs, AGENTS, CMake point to `seaengine/editor/` for sources (and `seaengine/external/` for vendored).

### Files/References Updated During Migration
- `.gitmodules`
- `CMakeLists.txt` (root + seaengine/editor/*)
- `docs/seaengine-editor-implications.md` (this section)
- `AGENTS.md` (build commands + Editor Modes + migration history)
- `seaengine/editor/external/CMakeLists.txt` (decoupled for vendoring)
- (Historical `tools/editor/AGENTS.md` and README removed with the directory)

The physical deletion of `tools/editor/` is complete.

---

## Open Questions (for discussion)

- How far should we push "low-chrome explorer mode" in the editor (almost full-screen viewport with minimal panels)?
- Should atmosphere/audio authoring be a dedicated panel early, or start as inspector extensions?
- What does multi-scale authoring actually look like in practice (separate "ocean map" view, hierarchical zones, etc.)?
- How much should the editor try to *replicate the final emotional experience* vs. being an efficient construction tool?
- For the blue whale phase later: will we need "creature view" authoring modes (sensory simulation, scale awareness)?

---

*This document should evolve alongside the vision as we learn what it actually takes to author this kind of experience.*