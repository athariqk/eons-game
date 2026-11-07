# Spec: Editor Edit / Simulation / Runtime Split

**Status:** Draft (2026-11-06)
**Owner:** (to be assigned)
**Related:** Editor plugin (`tools/editor/`), Scene / EcsWorld lifecycle

## 1. Overview & Goals

Introduce a proper, explicit split between **scene editing**, **in-editor simulation**, and **runtime preview** for the editor.

Key requirements:
- Simulation (play) **must still allow live editing** — inspector changes, gizmos, scene tree operations, and asset hot-reloads must immediately affect the running world.
- On "Stop" (returning from simulation to edit): **revert to the pre-play authored state** (snapshot before entering simulation; restore on stop).
- Pause **only gameplay logic** when not simulating. Engine systems (rendering prep, transforms, FPS, asset pipeline, editor UI) continue to run for responsiveness.
- Provide an **in-editor Runtime Preview** mode that behaves like a player experience inside the editor.
- Standalone runtime (no editor) remains the orthogonal case of simply not registering the editor plugin.

This replaces the current "world is always simulating" hardcoded state.

## 2. Current State

- Editor is DEBUG-only, registered via `register_editor_plugin(Scene&)` from game `on_ready()`.
- Single `Scene` + `EcsWorld` holds both authored content and live state.
- All edits are direct live mutations (via `PropertyGrid` drawers writing through RTTI pointers + `notify_changed`, gizmos, `queue_task`/`queue_destroy_node`).
- No `EditMode`, no time scale, no pause concept.
- `ToolbarPanel` has a disabled "▶ Simulating" button + tooltip advertising live editing.
- Editor systems are mixed into the normal phase pipeline.
- `Scene::on_variable_update` unconditionally calls `ecs_world.progress(real_dt)`.
- `TimeComponent` only tracks FPS/ticks.
- Separate "Scene View" (editor camera + authoring overlays) and "Game View" (active game camera) already exist.
- Live asset editing during "simulation" is already a stated goal in `editor_assets.*`.

## 3. Modes

```cpp
enum class EditMode {
    Edit,              // Authoring mode. Simulation is paused (sim_dt = 0).
    Simulate,          // Playing in editor. Full (or scaled) simulation. Live edits allowed.
    RuntimePreview     // In-editor player preview. Full simulation, authoring UI suppressed.
};
```

Controller operations (on `SimulationController` owned by `Editor`):
- `play()` / `enter_simulate()`
- `pause()`
- `stop()`  → revert snapshot, return to Edit
- `enter_runtime_preview()`
- `exit_runtime_preview()`

Transitions:
- Edit → Simulate: take snapshot of authored state.
- Simulate → Edit (Stop): restore snapshot.
- Simulate ↔ Paused (internal to Simulate).
- Any → RuntimePreview and back (view mode, no snapshot/revert semantics).

## 4. Architecture

### 4.1 Ownership & Singletons

- `SimulationController` lives in the `Editor` (tools/editor side).
- New or extended singleton(s) in the ECS world (visible to game + engine code):
  - Extend `TimeComponent` (or add companion `SimulationState` singleton) with:
    - `double wall_dt`
    - `double sim_dt` (0 when paused or Edit)
    - `double simulation_time`
    - `float time_scale`
    - `bool paused`
    - `EditMode mode`

A new early system (registered by the editor plugin) — "SimulationGate" or "SimulationTime" — runs in `PRE_UPDATE` and writes the effective `sim_dt` based on controller state + real delta.

### 4.2 Progress & dt Handling

- `Scene::on_variable_update` continues to call `ecs_world.progress(real_wall_dt)` (or near it) in all modes.
  - This keeps rendering, camera updates, editor UI, asset pump, ImGui, etc. smooth.
- Gameplay systems are expected to read `sim_dt` (from the singleton) or early-return when `sim_dt <= 0`.
- Engine/core systems (render prep, basic transforms, etc.) continue using wall/real time.
- Editor systems (UI, gizmos, free-cam) use wall time or ImGui `io.DeltaTime` / `MouseDelta` directly (already largely true for camera controls).

### 4.3 Live Editing in Simulation

All current live-edit paths remain **unconditional**:
- Inspector property writes (direct to component memory + notify).
- Gizmo transforms.
- Scene tree (spawn, reparent, rename, delete via queued tasks).
- Asset browser drag-drop + hot reload via `EditorAssets::pump()` + `begin_frame()`.

These mutations always hit the live ECS world, even in Simulate / RuntimePreview.

### 4.4 Snapshot & Revert (Edit ↔ Simulate)

On `Edit → Simulate`:
- Capture a reversible snapshot of the current authored scene state.

On `Simulate → Edit` (Stop):
- Restore the snapshot (revert mutations made while simulating).
- Clear any transient simulation state (e.g. spawned entities that should not persist).

Scope of snapshot (initial):
- Root + authored node tree.
- Registered RTTI components on those nodes.
- References (`Ref<T>`) via asset paths or loader identities.

Out of scope / challenges (documented):
- Runtime-spawned entities created during play.
- Non-RTTI or non-serializable state (GPU RIDs, future physics bodies, etc.).
- External resource mutations that should be considered "live" vs. authored.

Implementation options (to be prototyped):
- In-memory serialization using existing `TypeInfo` / `RecordInfo` + field iteration.
- Lightweight node + component deep copy (respecting `NodeRefComponent`, skipping editor-internal nodes via `is_editor_internal_node`).
- Temporary archive or a hidden "edit buffer" world.

The snapshot is stored inside `SimulationController` (or an associated buffer). Restore uses `queue_task` + recreation to stay within the structural-change rules.

RuntimePreview does **not** participate in snapshot/revert.

### 4.5 Input & Camera Routing

- Editor camera (free-cam controls) remains decoupled and always available in Edit/Simulate (decide per preview).
- Game input (`InputComponent`, `ActiveCameraTag`) should be active primarily when in Simulate / RuntimePreview and the Game View has focus.
- Existing UI capture flags (`set_ui_capture`) are extended with mode awareness.
- In RuntimePreview, authoring capture (gizmos, panels) is minimized.

### 4.6 Editor Systems per Mode

- Core editor systems (dockspace, UI draw, capture, canvas pass) remain registered.
- Authoring overlays (`GizmoOverlay`, `GridOverlay`, selection visuals) are suppressed or read-only in RuntimePreview.
- Simulation gate system is always present while the editor plugin is loaded.
- Free-cam updater and asset pump run in all modes.

## 5. UI Changes

### ToolbarPanel
- Replace disabled "▶ Simulating" with real controls:
  - ▶ Play (Edit → Simulate)
  - ⏸ Pause (within Simulate)
  - ■ Stop (Simulate → Edit + revert)
  - Separate affordance for "Runtime Preview" (enter/exit).
- Visual feedback: toolbar tint, "SIMULATING" badge, disabled state for certain authoring widgets when appropriate.
- Gizmo mode / wireframe controls remain available during Simulate (live edit).

### Viewport & Overlays
- Scene View: full authoring tools in Edit + Simulate.
- Game View: emphasized in Simulate / RuntimePreview.
- In RuntimePreview: hide or dim authoring overlays (gizmos, grid, selection, most stats). Focus on clean game rendering.

### Other Panels
- Inspector, Scene Tree, Assets: continue to allow live edits in Simulate.
- In RuntimePreview: optional suppression of heavy authoring UI (or auto-hide most panels).
- Layouts per mode can be saved/restored (future polish).

Menu bar can expose Simulation > Play / Pause / Stop / Runtime Preview.

## 6. Runtime Preview

- In-editor mode that approximates the final player experience.
- Full simulation speed (`time_scale = 1`, not paused).
- Game View is primary / maximized.
- Editor panels mostly hidden or reduced.
- No (or minimal) authoring overlays and selection.
- Still inside the same world + editor plugin (for quick exit back to authoring).
- Does not take or restore snapshots (it is a view, not a "play session" with revert).

Standalone runtime remains: build/run the game executable without the editor registration path.

## 7. Files & Integration Points

### New / Major Editor Changes
- `tools/editor/src/simulation/simulation_controller.h|.cpp` (or colocated in `editor.*`)
- `tools/editor/src/editor.h|.cpp` — own controller, expose mode + effective dt
- `tools/editor/src/editor_frame.h` — optionally carry mode snapshot for panels
- `tools/editor/src/ncore_editor.cpp` — register simulation gate system + controller wiring
- `tools/editor/src/panels/toolbar_panel.*`
- `tools/editor/src/panels/viewport_panel.*` + overlays
- `tools/editor/src/editor_factory.cpp` — construction / dependencies
- `tools/editor/AGENTS.md` — update architecture diagram and invariants

### Engine / Shared
- `ncore/include/ncore/runtime/components.h` — extend `TimeComponent`
- `ncore/src/runtime/scene_plugins.cpp` (or new core system) — default simulation time handling
- `ncore/src/runtime/scene.cpp` — minor if any (prefer to keep control in editor + singletons)
- Possibly a small helper in `EcsIterState` or a query helper for "get_sim_dt()"

### Game / Example
- Update `eons-game/main.cpp` (TestScene systems) to demonstrate using `sim_dt` where relevant.
- Future game systems should follow the convention.

## 8. Invariants (Must Not Break)

1. World is read-only inside systems — structural work always queued.
2. Editor component registration order (before any editor queries).
3. Main-thread surface for `ResourceLoader`, `RenderService`, `EditorAssets`, `ThumbnailCache`.
4. Panels: stable deps in ctor, per-frame state only via `EditorFrame`.
5. No `NCAPI` on template classes.
6. Editor camera remains decoupled from generic input handlers.
7. Live asset editing + shader hot-reload continues to work while simulating.
8. Entity deletion safety (use `queue_destroy_node`, destroy child queries first).

## 9. Risks & Open Questions

- Snapshot completeness and cost for large or dynamic scenes.
- How much "live" state during play (newly spawned objects, modified resources) should survive Stop vs. be discarded.
- Exact convention for game code to read simulation time (singleton lookup vs. passed delta).
- Interaction with future physics plugin (Box2D etc.).
- Whether fixed-step (`on_fixed_update`) needs the same split.
- Performance of snapshot/restore (may need incremental or sparse representation).
- Undo/redo (future) will need to integrate with the mode + snapshot story.
- Multiplayer / deterministic replay implications (out of scope for v1).

## 10. Implementation Phases (Suggested)

1. **Foundation**
   - Define `EditMode` + `SimulationController` (stubs + state machine).
   - Extend `TimeComponent`.
   - Implement simulation gate system that writes `sim_dt`.
   - Wire into `Editor` and editor plugin registration.
   - Make editor UI / cam use wall time where needed.

2. **Basic Mode UI & Transitions**
   - Toolbar buttons + mode changes.
   - Gate gameplay dt (prove by updating one or two example systems).
   - Pause within Simulate.

3. **Snapshot & Revert**
   - Design and implement pre-play snapshot.
   - Wire snapshot on Play, restore on Stop.
   - Handle editor-internal nodes and basic components.
   - Document limitations.

4. **Runtime Preview**
   - Enter/exit mode.
   - Panel suppression + Game View emphasis.
   - Overlay suppression.
   - Full-sim dt behavior.

5. **Polish & Documentation**
   - Input routing refinements.
   - Update game example code to respect sim time.
   - Update `tools/editor/AGENTS.md` and any runtime docs.
   - Smoke test (edit → play → edit with revert → runtime preview).
   - Add notes on how future systems (physics, scripting) should behave.

6. **Future (out of scope for initial cut)**
   - Standalone "Play" launcher button.
   - Better serialization for snapshots.
   - Per-system "gameplay" tagging if dt convention proves insufficient.
   - Undo integration.

## 11. Verification

- Build succeeds.
- Runtime smoke: editor loads, Scene/Game views render, basic editing works.
- Mode test:
  - Enter Simulate → game systems advance, live edits (transform, material, spawn) take effect immediately.
  - Stop → scene reverts to pre-play state (authored objects restored, play-time spawns removed).
  - Pause → gameplay halts, engine + editor remain responsive.
  - Runtime Preview → clean game view, full sim speed, authoring tools hidden.
- No regressions in asset live reload, selection, or camera controls.
- Logs and ASan clean during transitions.

---

This spec captures the agreed direction. Implementation issues should be filed under `.scratch/editor-simulation-modes/issues/`.