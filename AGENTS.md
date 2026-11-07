# AGENTS.md

Guidance for AI coding agents working in this repository.

## First Reads

1. `ncore/include/ncore.hpp` — umbrella header, shows all public includes
2. `ncore/include/ncore/application.h` — Application + AppDesc + lifecycle
3. `ncore/include/ncore/runtime/scene.h` — Scene: the default IGameWorld
4. `ncore/include/ncore/runtime/node.h` — Node: thin wrapper over ECS entity
5. `ncore/include/ncore/runtime/ecs/ecs_world.h` — EcsWorld: Flecs wrapper
6. `ncore/include/ncore/runtime/ecs/ecs_system.h` — SystemDelegate + EcsSystemBuilder
7. `ncore/include/ncore/runtime/ecs/ecs_query.h` — EcsTableIterator, QueryContext, EcsQueryBuilder
8. `ncore/include/ncore/runtime/ecs/ecs_entity.h` — EcsEntity: the 64-bit entity ID type
9. `ncore/include/ncore/core/types.h` — RTTI: TypeInfo, RecordInfo, FieldInfo, NSTRUCT_V
10. `ncore/include/ncore/services/service.h` — IService base + NullService
11. `ncore/include/ncore/services/servicedb.h` — ServiceDB (service locator)
12. `CMakeLists.txt` (root + ncore/)
13. `eons-game/main.cpp` — legacy prototype entry point
14. `seaengine/main.cpp` — primary SeaEngine entry (Explore vs --editor)
15. `seaengine/CMakeLists.txt` — now contains the merged editor sources list + external/ vendoring
16. `seaengine/seaengine.ini` — unified config (used for both Explore Mode (default) and Editor mode via `--editor`)

## Build And Run (Windows)

```
# Configure
cmake --preset windows-debug

# Build (ninja — preferred)
ninja -C build/windows-debug seaengine

# Full (including editor module)
ninja -C build/windows-debug ncore_d.dll seaengine EonsPrototype_d.exe

# Build (CMake)
cmake --build build --config Debug
```

**Run SeaEngine (default = Explore Mode):**
```
.\build\windows-debug\bin\Debug\seaengine
```

**With editor mode (`--editor`):**
```
.\build\windows-debug\bin\Debug\seaengine --editor
```

### Editor Modes

SeaEngine is a **single executable** (CMake target `seaengine`, app name "SeaEngine").

- **Explore Mode** (default): Immersive, SpaceEngine-like experience. Editor UI, panels, dockspace, and editor-related ECS systems are disabled. Pure exploration / free-cam focus.
- **Editor mode**: Activated via:
  - CLI: `seaengine --editor`
  - In-app: (stub) F12 prints activation instructions; full runtime menu toggle planned.
- Editor-related systems are **disabled** when in Explore Mode (no ImGui, no authoring overhead).
- Editor code (under `seaengine/editor/`) is **always compiled in** (not Debug-only) but inactive by default. It is included as plain sources directly in the single `seaengine` target (no separate library).
- When `--editor` is passed, the app uses the name "SeaEngine Editor" (while still loading the single unified `seaengine.ini`). Larger editor-friendly window defaults can be set in seaengine.ini; logging uses the [Log] section (default: seaengine.log).

Notes:
- Debug builds append `_d` per `ncore/CMakeLists.txt:35`.
- Uses the single unified `seaengine.ini` for both Explore Mode (default) and Editor mode (`--editor`); copied at build. (The separate seaengine_editor.ini is no longer used.)
- Assets are compiled to `.bin` by the `asset_pipeline` target (`cmake/CompileAssets.cmake`,
  driven by `resource_compiler.exe`). Raw originals (`.slang .png .jpg .jpeg .wav .ttf .otf .webp`)
  are never staged into `bin/.../assets` — only `<name>.bin` lands there. Slang `#include`
  resolution reads the raw sources straight from `ncore/assets`, `eons-game/assets` and
  `seaengine/assets` via `--search-paths`. `.ini` files are still copied at build time.
- Release: `cmake --preset windows-release`, target `ncore.dll seaengine EonsPrototype_d.exe`.
- Do NOT delete or rebuild `build/`. The existing build is canonical — full reconfigure is slow.
- The prototype game target is `EonsPrototype_d.exe`: `ninja -C build/windows-debug EonsPrototype_d.exe`.
- Main target `seaengine` (includes editor module from `seaengine/editor/`). Editor code always compiled in.
- Linking may fail with `machine type x86 conflicts with x64` if the shell's `LIB`
  env var points at the x86 MSVC libs. Use the x64 LIB paths for the build:
  `$env:LIB = "...\VC\Tools\MSVC\<ver>\lib\x64;...\Windows Kits\10\lib\10.0.28000.0\ucrt\x64;...\um\x64"`.

## Editor Migration — Removal Steps (Historical / Completed)

**Status:** The `tools/editor/` directory (old ncore_editor location + original submodule trees) has been removed from the working tree and index. The checklist below is preserved as the record of the migration that was performed.

Current state:
- `tools/editor/` no longer exists.
- All editor code is under `seaengine/editor/` (flattened; no `src/` subdir). Vendored externals under `seaengine/external/`.
- Single `seaengine` target only.
- No references to old plugin registration or ncore_editor in active build/runtime code.

**Verification performed before `git rm`:**
- `ninja -C build/windows-debug seaengine` succeeds cleanly.
- SeaEngine launches in **Explore Mode** (default) and editor works via `--editor` or in-app menu.
- No compile/runtime references to `ncore_editor`, `tools/editor`, or `register_editor_plugin` remain in active code.
- `seaengine/external/` trees are clean (no `.git` files inside). Editor sources flattened directly under `seaengine/editor/`.

### Submodule Migration Plan (Chosen: Clean Tree Copy)
- **Approach taken:** Vendored source trees copied into `seaengine/editor/external/` (`.git` pointers and submodule metadata stripped). No re-add as submodules.
- **Why not re-add submodules:** Reduces contributor friction (no `git submodule update --init` for editor deps), matches common practice for ImGui/ImGuizmo. External CMake already lists sources explicitly rather than relying on submodule CMake.
- **If upstream bump needed later:** Replace tree contents under `seaengine/editor/external/{imgui,imguizmo,freetype}` (preserve the local `imgui_config.h` and any patches in `external/CMakeLists.txt`).
- `.gitmodules` cleaned of the three `tools/editor/external/*` entries.
- The `seaengine/editor/external/` dirs are now regular committed content (will be `git add`ed post-clean).

### Exact Removal Steps (run from repo root, in order)
```powershell
# 1. Deinit the submodules (safe even if already partially cleaned)
git submodule deinit -f tools/editor/external/imgui tools/editor/external/imguizmo tools/editor/external/freetype

# 2. Remove the submodule working trees + index entries
git rm -r --cached tools/editor/external/imgui tools/editor/external/imguizmo tools/editor/external/freetype
git rm -r tools/editor   # this will also remove the (now-empty after deinit) tools/editor dir and its AGENTS.md / sources

# 3. Clean .gitmodules (already edited by migration; double-check)
#    (The edit removed the three [submodule "tools/editor/external/..."] blocks)

# 4. Remove stale git module metadata (the absorbed worktrees)
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue .git/modules/tools/editor

# 5. Stage the now-plain vendored external trees (they were untracked copies; now clean)
git add seaengine/editor/external/

# 6. Commit the removal + migration finalization
git commit -m "chore(editor): remove deprecated tools/editor/ (ncore_editor)

- Submodules (imgui, imguizmo, freetype) migrated as vendored trees to seaengine/editor/external/
- seaengine/editor/external/CMakeLists.txt decoupled for standalone use
- All references and docs updated; seaengine is the single SeaEngine target (Explore Mode by default)
See docs/seaengine-editor-implications.md#migration-complete"

# 7. Post-removal verification (build + smoke)
ninja -C build/windows-debug seaengine
# Run Explore Mode:
#   .\build\windows-debug\bin\Debug\seaengine
# Run with editor:
#   .\build\windows-debug\bin\Debug\seaengine --editor
# Confirm no "tools/editor" or "ncore_editor" in build output or logs.
```

### Optional Cleanup (aggressive — only if sure)
```powershell
git clean -fdx   # WARNING: removes ALL untracked (use with care; will nuke build/ if present)
```

**Result after executing the steps + commit:**
- `tools/editor/` no longer exists on disk or in the repository.
- Old `tools/editor/AGENTS.md` (historical guide) was deleted with the dir.
- All future editor work references only `seaengine/editor/` (sources) and `seaengine/external/` (vendored).

See also: `docs/seaengine-editor-implications.md` (full "Migration Complete (Nov 2026)" section).

Note: As of now, `seaengine/editor/AGENTS.md` does not exist. Editor guidance lives in:
- `seaengine/CMakeLists.txt` (editor sources + external vendoring comments)
- `docs/seaengine-editor-implications.md`
- `docs/seaengine-vision.md`
- Root `AGENTS.md` (this file)
- Code comments in `seaengine/editor/`

After `tools/editor/` removal, a lightweight `seaengine/editor/AGENTS.md` may be added for future editor contributors.

## Debug = AddressSanitizer (+ leak reporter)

- The Debug config compiles everything with `/fsanitize=address` (clang-cl).
  clang-cl ASAN requires the release CRT, so Debug uses `/MD` (Release/Dist keep
  their normal CRT via `CMAKE_MSVC_RUNTIME_LIBRARY`).
- `clang_rt.asan_dynamic-x86_64.dll` is copied next to the exe at build time —
  the exe won't start without it.
- Under ASAN (`NC_ASAN_ENABLED`), the engine bypasses mimalloc: `new`/`delete`
  stay default (ASAN-intercepted) and `memalloc*` route to `malloc`/`free`
  family (`ncore/src/core/memory.cpp`); `alloc_overrides.h` is a no-op.
- Windows has no LeakSanitizer, so `ncore/src/core/asan_leaks.cpp` installs the
  sanitizer malloc/free hooks and prints a leak report at exit (stderr +
  debugger output), grouped by allocation stack and symbolized to
  `function file:line` via dbghelp.
- Useful `ASAN_OPTIONS`: `fast_unwind_on_malloc=0:malloc_context_size=20`
  (full allocation stacks for the leak report). Symbolization needs
  `ASAN_SYMBOLIZER_PATH=C:/tools/LLVM/bin/llvm-symbolizer.exe` (already set in
  `VS_DEBUGGER_ENVIRONMENT` for EonsPrototype).
- `detect_leaks` is NOT supported on this platform — the custom leak reporter
  replaces it. Memory errors (OOB/UAF/double-free) print symbolized reports at
  the moment they happen.

## Runtime Logs & Crash Diagnosis

- SeaEngine (Explore or Editor): `build\windows-debug\bin\Debug\logs\seaengine.log`. Configured via the [Log] section in the (single unified) copied `seaengine.ini`. (The editor mode shares the same log file as Explore Mode.)
- eons-game prototype: `build\windows-debug\bin\Debug\logs\eons.log` (from `eons.ini`).

```
# Tail logs live while the app runs (SeaEngine)
Get-Content -Path "build\windows-debug\bin\Debug\logs\seaengine*.log" -Wait -Tail 20

# Search for errors across all log files
Select-String -Path "build\windows-debug\bin\Debug\logs\*.log" -Pattern "ERROR|sentinel|assert|SEA|editor" | Select-Object -Last 30

# Post-crash: search for the crash location
Select-String -Path "build\windows-debug\bin\Debug\logs\seaengine*.log" -Pattern "ECS.*ERROR|sentinel|Access violation|FATAL"
```

Auto-run + tail (PowerShell) for SeaEngine:
```
$log = "build\windows-debug\bin\Debug\logs\seaengine.log"
$job = Start-Job { Get-Content $using:log -Wait -Tail 10 }
Start-Process -FilePath "build\windows-debug\bin\Debug\seaengine_d.exe" -Wait
$job | Receive-Job | Select-Object -Last 20
Remove-Job $job
```

## Architecture Summary

### Directory layout

```
ncore/                          Engine library (shared, ncore_d.dll)
  include/ncore/
    core/                       Foundation: RTTI types, math (Vec2/3/4, Quaternion, Mat4),
                                containers (PagedPool, RIDPool, RingBuffer),
                                memory, Ref<T> (intrusive ref-counted), RID, Color, Rect, errors.
    resources/                  Resource types (Mesh, Image, Shader, MaterialTemplate, AudioClip).
    runtime/                    ECS runtime + Scene/Node.
      ecs/                      EcsWorld, EcsTableIterator, EcsSystemBuilder, EcsQueryBuilder.
      components/               Component structs (Transform, Camera, Mesh, Sprite, etc.).
      scene.h / node.h          Scene graph layered over ECS.
    services/                   Engine services (IService + ServiceDB).
      audio/, events/, input/, io/, physics/, video/.
    utils/                      Assert, log, config.
  src/
    runtime/                    ECS + Scene + Node implementations.
      scene_plugins.cpp/.h      Engine subsystem plugins (window, render, gui, input).
    services/                   Service implementations.
    backends/                   SDL, Diligent, Box2D, Flecs helpers.
    application.cpp             Application lifecycle (init -> run -> finish).

eons-game/                      Legacy/prototype game project (executable: EonsPrototype_d.exe).
  main.cpp                      GameApplication + TestScene subclass.
  src/microcosmos/              Game-specific ECS systems + components.

seaengine/                      Primary SeaEngine application (single executable `seaengine`).
  main.cpp                      Entry: Explore Mode (default, WorldScene) or Editor via `--editor`.
  src/                          Ocean/water sim + WorldScene (pure immersive; systems off).
  editor/                       Editor module (always compiled in as plain sources into the single seaengine target).
    CMakeLists.txt              Documents "single binary, editor code always compiled".
    src/ + external/            Editor panels, EditorWorldHost, vendored ImGui etc.
```

### Key abstractions

**ServiceDB** — service locator for engine subsystems (WindowService, RenderService,
InputService, etc.). All inherit `IService`. Registered in `Application::register_services()`
via `services.provide<T>()`.

**Scene** — default `IGameWorld`. Owns an `EcsWorld` + a `NodePool` + a root `Node`.
Lifecycle: `on_enter()` loads engine plugins via `register_*_plugin()`, then calls
`on_ready()` (game hook), then `ecs_world.finalize_ordering()`. Every frame,
`on_variable_update(dt)` processes pending deletes then calls `ecs_world.progress(dt)`.

**Node** — thin view into the ECS. Wraps an `EcsEntity`. Provides `create_child()`,
`get_children()`, `get_child_count()`, `add_component<T>()`, `get_component<T>()`,
`remove_component<T>()`. Children are queried via a cached `EcsQuery` built
programmatically with a `(ChildOf, self_id)` pair term.

**NodeRefComponent** — `{Node* node}` component placed on every Node-wrapped entity.
Used by `ChildRange::Iterator` to resolve entity -> Node*. Skip in Inspector display.

**EcsWorld** — Flecs v4 C API wrapper. `system(name)`, `query(name)`, `observer(name)`
return fluent builders. `progress(dt)` ticks all systems. `finalize_ordering()`
sorts systems within pipeline phases.

**EcsSystemBuilder** — fluent: `.with<T>()`, `.in(phase)`, `.order(n)`,
`.each(fn)` / `.run(fn)`. Both callbacks take a single `EcsIterState&` — `run`
invokes once per table, `each` once per entity (row pre-set; current entity via
`ctx.entity()`). Callbacks accept any callable (templated). Zero-allocation
path for raw function pointers (`+[](...) { ... }`).

**SystemDelegate<Fn>** — heap-allocates captured lambdas. Provides static
`invoke_run(void*)` / `invoke_each(void*)` trampolines that recover the delegate
via `EcsIterState::user_ctx()`. Typed `destroy()` for cleanup.

**EcsTableIterator** — table-level input iterator (range-for compatible over EcsQuery).
Owns `ecs_iter_t*` (heap-allocated, deleted in dtor). `operator++()` calls
`ecs_query_next()`. Exposes `count()`, `entity(row)`, `user_ctx()`,
`get_internal_iter()`. Destructor calls `ecs_iter_fini()` if not fully
consumed. Loop variable must be `auto&` (`for (auto& it : query)`) — the
cursor is move-only. Equality is only meaningful against the end sentinel.

**EcsEntityIterator / EcsEntityView** — per-entity iteration over a query:
`query.entities()` returns an `EcsEntityView`; `for (auto ctx : query.entities())`
yields a copyable `EcsIterState` (row pre-set) for every matched entity,
walking rows across tables. Loop variable is a value, not a reference.

**EcsIterState** — row-level component accessor. Takes raw `void*` (ecs_iter_t*).
`set_row()`, `get_component<T>()`, `count()`, `entity()` (current row) /
`entity(row)`, `user_ctx()`, `mark_component_modified<T>()`. No Flecs types in
public header.

**EcsQueryBuilder** — `.with<T>()`, `.with_pair(EcsEntity, EcsEntity)`, `.up()`,
`.self()`, `.expr(dsl)`, `.build()`. `with_pair` takes `EcsEntity` (uint64_t)
so no Flecs types leak into public headers.

**RTTI (types.h)** — `NSTRUCT_V(T)` + `NC_PROPS_BEGIN()` / `ADD_PROPERTY(m)` /
`NC_PROPS_END()` auto-registers types at static init.
`NCLASS(T, Parent)` for class types inheriting `Object`. Field names are bare
identifiers; default flags are SERIALIZABLE|EDITABLE. Use
`ADD_PROPERTY(m, flag)` for custom flags on individual fields. `TypeInfo` has
`kind` (exact-width `TypeKind`: BOOL, INT8..UINT64, FLOAT, DOUBLE, STRING,
POINTER, ENUM, RECORD, VECTOR) + non-virtual predicates (`is_floating()`,
`is_record()`, `is_enum()`, `is_string()`, `is_container()`, `is_primitive()`)
and a kind-dispatched `to_string()`. Subclass ctors set kind
(RecordInfo→RECORD, EnumInfo→ENUM, StringClass→STRING, VectorClass→VECTOR);
primitives infer it via `detail::kind_of<T>()` through `TTypeInfo<T>`
registration. `FieldInfo` has no category — derive it from
`field.get_type()->kind` + `field.qualifier`. `ADD_PROPERTY` lambda-based
decomposes `T* m` / `T[N] m` / `char* m` into the qualifier (pointer_count,
array_length, is_cstring) and stores the pointee/element type id.
Primitives registered in `TypeDB::initialize()` in `ncore/src/core/types.cpp`.

## Code Boundaries

- Engine code is under `ncore/` (namespace `nc`). Keep reusable/generic.
- Game code is under `eons-game/`. Game-specific systems and components live there.
- Do not edit vendored libraries in `ncore/external/`.
- Do not edit `build/` directory contents.
- Prefer CMake. Legacy Premake files exist but are unused.

## Safe Editing Rules

- Add new `.cpp` files to `ncore/src/CMakeLists.txt`. Add new public headers to
  `ncore/include/CMakeLists.txt`.
- Use `cmake --preset windows-debug` only when the configure stamp is stale.
- Prefer `ninja -C build/windows-debug <target>` for incremental builds.
- **NCAPI on template classes is forbidden.** `Ref<T>`, `PagedPool<T>`,
  `RIDPool<T>`, `RingBuffer<T>`, `SlotIterator`, `CommonVectorOps`,
  `VectorClass` are all header-only templates and must NOT have NCAPI
  (DLL export/import). Doing so causes linker errors when game code
  instantiates them.

## Known Issues / Gotchas

- **Entity deletion during iteration**: Flecs locks entities referenced by active
  queries. Before deleting a node, destroy its cached child_query via
  `EcsWorld::destroy_query(name)`. Always use `Scene::queue_destroy_node()`
  for deferred safe deletion (processed in a PRE_FRAME system inside
  `ecs_progress()`).

- **Get_child_count** iterates all children via the child query. Avoid in
  tight loops. Fine for editor tree display.

- **ecs_children() vs ecs_query_next()**: These use completely different Flecs
  advancement functions. `EcsTableIterator::operator++()` only calls `ecs_query_next()`.
  Never wrap `ecs_children()` iterators in `EcsTableIterator`.

- **EcsCoreEvent constants**: Defined in `ecs/ecs_events.h` — use `EcsCoreEvent::OnSet`,
  `EcsCoreEvent::OnRemove`, etc. with `observer().on<T>(event)`.

- **Query expressions with numeric IDs**: Flecs DSL can't resolve bare numeric
  entity IDs in expression strings (e.g. `"NodeRefComponent, (ChildOf, 554)"`).
  Use the programmatic builder `.with<NodeRefComponent>().with_pair(EcsChildOf, id)`
  instead.

- **RID registration**: `rid.h` is transitively included before the `NSTRUCT_V`
  macro is defined in `types.h`. RID is therefore registered manually in
  `TypeDB::initialize()` via `TRecordInfo<RID>`.

## Component / RTTI Conventions

- All component structs should have `NSTRUCT_V(T)` + `NC_PROPS_BEGIN(T)` / `ADD_PROPERTY(m)` / `NC_PROPS_END(T)`.
- `NSTRUCT_V` auto-registers at static init via `TypeDB::register_type<TRecordInfo<T>, T>(#T)`.
- Primitives are registered in `TypeDB::initialize()` in `ncore/src/core/types.cpp`.
- If a header is transitively included by `types.h` before the `NSTRUCT_V` macro
  definition (line ~732), use manual registration in `initialize()` instead.

## Build-Only Verification

No automated test suite at root. Build success + runtime smoke check is the verification step.

- `ninja -C build/windows-debug seaengine`
- Run Explore Mode (default): `.\build\windows-debug\bin\Debug\seaengine` — clean immersive, no editor UI/systems.
- Run Editor: `.\build\windows-debug\bin\Debug\seaengine --editor` — panels, scene tree, inspector, etc. functional.
- Smoke expectations: main free-cam works in Explore; editor shows dockspace + viewport + inspector when `--editor`.

## Agent skills

### Issue tracker

Issues and specs live as markdown files under `.scratch/` in this repo. See `docs/agents/issue-tracker.md`.

### Domain docs

Single-context layout: one `GLOSSARY.md` + `docs/adr/` at the repo root. See `docs/agents/domain.md`.
