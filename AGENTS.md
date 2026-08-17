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
9. `ncore/include/ncore/core/types.h` — RTTI: TypeInfo, RecordInfo, FieldInfo, NSTRUCT
10. `ncore/include/ncore/services/service.h` — IService base + NullService
11. `ncore/include/ncore/services/service_registry.h` — ServiceRegistry (service locator)
12. `CMakeLists.txt` (root + ncore/)
13. `eons-game/main.cpp` — game entry point

## Build And Run (Windows)

```
# Configure
cmake --preset windows-debug

# Build (ninja — preferred, avoids CMake re-configure flakiness)
ninja -C build/windows-debug ncore_d.dll ncore_editor_d.dll eons-game_d.exe

# Build (CMake)
cmake --build build --config Debug

# Run
.\build\windows-debug\bin\Debug\eons-game_d.exe
```

Notes:
- Debug builds append `_d` per `ncore/CMakeLists.txt:35`.
- `ncore_editor_d.dll` is a separate shared library under `tools/editor/` — always build it alongside `ncore_d.dll`.
- Assets and `.ini` files are auto-copied at build time.
- Release: `cmake --preset windows-release`, target `ncore.dll ncore_editor.dll eons-game.exe`.
- Do NOT delete or rebuild `build/`. The existing build is canonical — full reconfigure is slow.
- The game target is `EonsPrototype_d.exe` (not `eons-game_d.exe`): `ninja -C build/windows-debug EonsPrototype_d.exe`.
- Linking may fail with `machine type x86 conflicts with x64` if the shell's `LIB`
  env var points at the x86 MSVC libs. Use the x64 LIB paths for the build:
  `$env:LIB = "...\VC\Tools\MSVC\<ver>\lib\x64;...\Windows Kits\10\lib\10.0.28000.0\ucrt\x64;...\um\x64"`.

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

Log file: `build\windows-debug\bin\Debug\logs\eons.log` (configured in `eons.ini`).

```
# Tail logs live while the app runs
Get-Content -Path "build\windows-debug\bin\Debug\logs\eons.log" -Wait -Tail 20

# Search for errors across all log files
Select-String -Path "build\windows-debug\bin\Debug\logs\eons.log*" -Pattern "ERROR|sentinel|assert" | Select-Object -Last 30

# Post-crash: search for the crash location
Select-String -Path "build\windows-debug\bin\Debug\logs\eons.log" -Pattern "ECS.*ERROR|sentinel|Access violation"
```

Auto-run + tail (PowerShell):
```
$log = "build\windows-debug\bin\Debug\logs\eons.log"
$job = Start-Job { Get-Content $using:log -Wait -Tail 10 }
Start-Process -FilePath "build\windows-debug\bin\Debug\eons-game_d.exe" -Wait
$job | Receive-Job | Select-Object -Last 20
Remove-Job $job
```

## Architecture Summary

### Directory layout

```
ncore/                          Engine library (shared, ncore_d.dll)
  include/ncore/
    core/                       Foundation: RTTI types, math (Vec2/3/4, Quaternion, Mat4),
                                containers (PagedPool, ResourcePool, RingBuffer),
                                memory, Ref<T> (intrusive ref-counted), RID, Color, Rect, errors.
    resources/                  Resource types (Mesh, Image, Shader, MaterialTemplate, AudioClip).
    runtime/                    ECS runtime + Scene/Node.
      ecs/                      EcsWorld, EcsTableIterator, EcsSystemBuilder, EcsQueryBuilder.
      components/               Component structs (Transform, Camera, Mesh, Sprite, etc.).
      scene.h / node.h          Scene graph layered over ECS.
    services/                   Engine services (IService + ServiceRegistry).
      audio/, events/, input/, io/, physics/, video/.
    utils/                      Assert, log, config.
  src/
    runtime/                    ECS + Scene + Node implementations.
      scene_plugins.cpp/.h      Engine subsystem plugins (window, render, gui, input).
    services/                   Service implementations.
    backends/                   SDL, Diligent, Box2D, Flecs helpers.
    application.cpp             Application lifecycle (init -> run -> finish).

eons-game/                      Game project (executable, eons-game_d.exe).
  main.cpp                      GameApplication + TestScene subclass.
  src/microcosmos/              Game-specific ECS systems + components.
```

### Key abstractions

**ServiceRegistry** — service locator for engine subsystems (WindowService, RenderService,
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

**RTTI (types.h)** — `NSTRUCTV(T, ...)` auto-registers types at static init.
`TypeInfo` has `kind` (exact-width `TypeKind`: BOOL, INT8..UINT64, FLOAT,
DOUBLE, STRING, POINTER, ENUM, RECORD, VECTOR) + non-virtual predicates
(`is_floating()`, `is_record()`, `is_enum()`, `is_string()`,
`is_container()`, `is_primitive()`) and a kind-dispatched `to_string()`.
Subclass ctors set kind (RecordInfo→RECORD, EnumInfo→ENUM, StringClass→STRING,
VectorClass→VECTOR); primitives infer it via `detail::kind_of<T>()` through
`TTypeInfo<T>` registration. `FieldInfo` has no category — derive it from
`field.get_type()->kind` + `field.qualifier`. `NC_F(T, m)` decomposes
`T* m` / `T[N] m` / `char* m` into the qualifier (pointer_count,
array_length, is_cstring) and stores the pointee/element type id. Primitives
registered in `TypeRegistry::initialize()` in `ncore/src/core/types.cpp`.

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
  `ResourcePool<T>`, `RingBuffer<T>`, `SlotIterator`, `CommonVectorOps`,
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

- **RID registration**: `rid.h` is transitively included before the `NSTRUCT`
  macro is defined in `types.h`. RID is therefore registered manually in
  `TypeRegistry::initialize()` via `TRecordInfo<RID>`.

## Component / RTTI Conventions

- All component structs should have `NSTRUCTV(T, NC_F(T, field1), NC_F(T, field2), ...)`.
- `NSTRUCT` auto-registers at static init via `TypeRegistry::register_type<TRecordInfo<T>, T>(#T)`.
- Primitives are registered in `TypeRegistry::initialize()` in `ncore/src/core/types.cpp`.
- If a header is transitively included by `types.h` before the `NSTRUCT` macro
  definition (line ~732), use manual registration in `initialize()` instead.

## Build-Only Verification

No automated test suite at root. Build success + runtime smoke check (main menu
visible, scene tree populated, inspector shows component data, camera/stats
windows toggle correctly) is the verification step.
