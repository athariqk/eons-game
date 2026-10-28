# ncore_editor — Agent Guide

Editor tooling for the engine. This is the agent-facing source of truth for
working in `tools/editor/`; `README.md` is the human-facing prose version. The
repo root `AGENTS.md` covers engine-wide rules (build, ASan, naming); this file
adds the editor-specific architecture, invariants and gotchas.

## What this is

- `ncore_editor_d.dll` — a Debug-only shared library. It is **not** the app: a
  game enables it from its own `Scene::on_ready()` under `#if defined(DEBUG)`.
- Entry points (exported with `NCAPI_EDITOR`, declared in `include/editor/ncore_editor.h`):
  `register_editor_plugin(Scene&)` / `unregister_editor_plugin(Scene&)`.
- The editor lives in the ECS world as a singleton and forwards through a small
  set of ECS systems; there is no editor global.

## Architecture

```
ECS world ── EditorSingleton { Editor* editor }
  ├─ EditorCamera_FreeCamUpdater (UPDATE)   — free-fly camera input
  ├─ EditorImGuiInit             (INIT)     — ImGuizmo context binding
  ├─ EditorImGuizmoBeginFrame    (PRE_UPDATE)
  ├─ EditorDockspace             (PRE_UPDATE)
  ├─ EditorUi                    (UPDATE)   — Editor::begin_frame() + draw()
  ├─ EditorUiCapture             (POST_UPDATE)
  └─ EditorCanvasPass            (POST_FRAME) — swapchain canvas pass

Editor (thin shell)
  ├─ create_panels()   — constructs every panel with constructor-injected deps
  ├─ begin_frame()     — fills the per-frame EditorFrame + importer->pump()
  ├─ draw()            — menu bar + panels in (slot, order) order
  └─ owns: EditorSelection, LogStore, EditorSettings,
           AssetDatabase, ImportService, ThumbnailCache

panels: EditorPanel::draw_window() owns the ImGui Begin/End; a concrete panel
  implements draw_contents() plus, when needed, window_flags() / pre_begin() /
  on_collapsed() / post_end(). draw_contents() runs only while the window is open.

asset pipeline:
  AssetDatabase (data + manifest + procedural registry; scan only enqueues hashes)
  ImportService (worker: compile + hash; main pump(): reload + commit + flush manifest)
  ThumbnailCache (GPU textures, main thread)
  AssetPaths (path resolution + hashing helpers)
```

Panels take **stable dependencies in their constructor** and read only
**per-frame state** from `EditorFrame`. `EditorFrame` contains exactly:
`ecs_world`, `renderer`, `inputs`, `time`, and the viewport hover/focus flags.

## File map

```
include/editor/ncore_editor.h   public entry points (register/unregister)
src/editor.h/.cpp                shell: Editor, EditorFrame, EditorPanel,
                                 DockSlot, LogStore, EditorSettings,
                                 InspectorHandler, PropertyWidget
src/editor_selection.h/.cpp      EditorSelection (std::variant<EcsEntity,String>)
src/editor_components.h          ECS components: EditorCameraTag, SelectedTag,
                                 EditorSingleton
src/ncore_editor.cpp             register/unregister, the ECS systems, free cam
src/plugins/gui_plugin.h/.cpp    ImGui backend (GuiStateComponent, init/teardown)
src/io/asset_paths.h/.cpp        AssetKind, AssetPaths, path resolution + hashing
src/io/editor_assets.h/.cpp      AssetOrigin/Status/Entry, ImportManifest,
                                 ProceduralRecord, AssetDatabase
src/io/asset_importer.h/.cpp     ImportService (worker: compile + scan hash), results
src/io/thumbnail_cache.h/.cpp    ThumbnailCache (GPU thumbnail LRU)
src/panels/toolbar_panel.*       gizmo mode / world-local / wireframe
src/panels/viewport_panel.*      offscreen viewport + Gizmo/Stats overlays
src/panels/scene_tree_panel.*    hierarchy, selection, reparent, spawn/rename
src/panels/inspector_panel.*     RTTI property sheet + handler/widget dispatch
src/panels/assets_panel.*        asset browser (list/grid)
src/panels/asset_picker.*        RID property widget (AssetPicker/RidHint)
src/panels/logs_panel.*          console view over LogStore
src/panels/material_inspector.*  material uniform inspector (InspectorHandler)
src/backends/imgui/              ImGui style + helpers
```

## Invariants (do not break)

1. **World is read-only inside editor systems.** Structural changes must be
   queued: `Scene::queue_task(...)` for creation, `Scene::queue_destroy_node(...)`
   for deletion. Inline entity creation returns deferred ids that poison pairs,
   queries and deletion.
2. **Register component types before any query.** `register_editor_component_types`
   (EditorCameraTag, SelectedTag, EditorSingleton, GuiStateComponent) must run
   before any editor query/system — Flecs refuses to apply `EcsCanToggle` traits
   after a component is referenced.
3. **Main-thread-only surface.** `ResourceLoader`, `RenderService`,
   `AssetDatabase` and `ThumbnailCache` are all main-thread-only. The
   `ImportService` worker may only touch `AssetService::compile` plus hashing
   (`hash_source_tree`/`hash_file`) — both reimports and scan-time reconciles
   run there. Its results are applied on the main thread in `ImportService::pump()`
   (`mark_imported`/`mark_scanned`, then `flush_manifest`). `AssetDatabase::scan`
   must never hash on the caller's thread: it enqueues `enqueue_hash` instead.
4. **Panels: constructor = stable deps, `draw_contents(EditorFrame&)` = per-frame
   only.** A panel's dependencies are visible in its constructor. Do not re-add
   services to `EditorFrame`. The shell owns `ImGui::Begin`/`End`; panels must
   not call them.
5. **Stored-reference constructor params need `[[clang::lifetimebound]]`**
   (`-Werror -Wlifetime-safety-cross-tu-constructor-suggestions`).
6. **No `NCAPI` on template classes** (`Ref`, `PagedPool`, `RIDPool`,
   `RingBuffer`, `VectorClass`, etc.) — game code instantiates them and link
   errors follow.

## Build & verify

Debug targets append `_d`. Build from the repo root:

```powershell
$env:LIB = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\atlmfc\lib\x64;C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\lib\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.28000.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.28000.0\um\x64"
ninja -C build/windows-debug ncore_editor_d.dll EonsPrototype_d.exe SeaEngine_d.exe
```

The `LIB` override matters: a stale shell `LIB` pointing at x86 MSVC libs yields
`machine type x86 conflicts with x64`.

Smoke test (editor work is Debug-only, so run the game, not a unit test):

```powershell
cd build\windows-debug\bin\Debug
$err = "$env:TEMP\smoke.txt"; Remove-Item $err -ErrorAction SilentlyContinue
$p = Start-Process -FilePath ".\EonsPrototype_d.exe" -PassThru -RedirectStandardError $err
Start-Sleep -Seconds 40            # ASan + Vulkan init is slow; the editor scans on the first UI frame
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; "alive" } else { "exited 0x{0:X8}" -f $p.ExitCode }
Select-String -Path $err -Pattern "FATAL|Assertion|AddressSanitizer"
Select-String -Path logs\engine.log -Pattern "AssetDatabase:" | Select-Object -Last 1
```

Pass = process alive, no assertion/ASan in stderr, and
`AssetDatabase: 18 assets` (16 file + 2 procedural) in the log. The log is
`logs/engine.log` (not `eons.log`); `ImportService: 3 source dirs, 6 shader
search paths` confirms the importer initialized.

## Gotchas (these actually bit)

- `ImGui::SetNextItemWidth(-FLT_MIN)` only on the **last** item of a row — in the
  middle it consumes the whole remaining width and pushes trailing widgets off-screen.
- `std::jthread` must be declared **after** the `std::mutex`/`std::condition_variable`
  it uses, so it joins before they are destroyed.
- `-Werror` turns these into hard errors: `-Wshadow`, `-Wctad-maybe-unsupported`
  (`std::lock_guard` needs an explicit `<std::mutex>`), and the
  `-Wlifetime-safety-cross-tu-*` family.
- `-Wexit-time-destructors` forbids a function-local `static` of a non-trivial
  type in a header (e.g. a `static const String empty;` sentinel) — return a
  `StringView` instead of a `const String&` sentinel, or hoist the state.
- `windows.h` must be included **after** ncore headers, with `NOMINMAX` +
  `WIN32_LEAN_AND_MEAN`, or it breaks `ncore/core/aabb.h`'s `max(`.
- Entity deletion during iteration: Flecs locks entities referenced by active
  queries — always defer deletes.
- `ImGui::Begin`/`End` must pair exactly; a window submitted after `ImGui::Render()`
  trips `g.WithinFrameScope`.

## Conventions

- Parameters prefixed `p_`; members `snake_case` with no trailing underscore.
- Accessors use `get_`; where a private member would collide with an API verb it
  takes an `_impl` suffix.
- Pointer/reference-returning members are `[[clang::lifetimebound]]`.
- New editor features arrive as panels registered in `Editor::create_panels()`.
