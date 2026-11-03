# Resource/Service Inversion — Implementation Plan

Status: Ready for implementation
Date: 2026-10-30
Area: `ncore` resource system, renderer API, ECS component layer, editor asset pipeline
Branch state: mid-rewrite working tree (does not compile)

---

## 1. Problem

The resource system is inverted. `IResource` objects are passive CPU data containers; the ECS
layer acts as the driver that injects them into `IService` modules. `register_video_plugin`
creates GPU objects, stores their RIDs in components, and destroys them on teardown, while
`RenderStorage` keeps `Ref<MaterialShader>` alive purely to read reflection data.

That inversion forces the service layer to depend on the runtime layer: `RenderService`,
`RenderStorage`, `WindowService` and `AudioService` all take resource types (`Image`, `Mesh`,
`Shader`, `MaterialShader`, `AudioClip`) in their public signatures.

We want:

1. `IResource` is the driver. It owns its CPU data **and** its live representation, and it talks
   to `IService` directly.
2. ECS holds references to resources plus genuinely entity-specific data (transform, camera,
   instance count, layer index), nothing else.
3. Layering is inverted: `runtime` depends on `services`, never the reverse.

## 2. Goals

- No header under `ncore/include/ncore/services/` includes `ncore/runtime/` (one documented
  exemption, see §3).
- The renderer accepts only neutral POD descriptors and spans, never resource types.
- `IResource` drives creation/release of its live representation through services.
- ECS components store `Ref<T>` references; no GPU-object bookkeeping in systems.
- Hot reload, device reset and shutdown all resolve through one mechanism, with no observer
  network.

## 3. Non-goals

- Async loading. `ResourceLoader::load` stays blocking.
- Scene/world serialization. Components get persisted references *only* if/when that lands.
- Moving serialization out of the asset compilers.
- Splitting the single `ncore` DLL into runtime/service libraries. The layering rule is
  enforced by review + an audit grep, not by the build.

## 4. Layering contract

**Rule.** A header or source file under `ncore/services/**` may include `core/**`, `utils/**`,
and `services/**`. It may not include `runtime/**`.

**Exception (documented in-file, at the top of `services/io/asset_compiler.h`).**
`AssetService`, `IAssetCompiler` and the import backends
(`ncore/src/backends/{stb,truetype,sdl,slang}/*_compiler.*`, `slang_module_compiler.*`) are
build-time/editor-only tooling. They construct resources and write `.bin` archives on a worker
thread with no `RenderService` and no main-thread affinity. They stay runtime-aware.

**Audit command** (must return only `asset_compiler.h`):

```powershell
Select-String -Path ncore/include/ncore/services/**/*.h -Pattern "ncore/runtime"
```

## 5. Decisions

| # | Decision | Rationale |
|---|---|---|
| D1 | Asset compilers are exempt from the layering rule. | Offline tooling; moving archive writing into runtime is a much larger refactor with no runtime benefit. |
| D2 | `IResource::get_rid()` is the handle of the resource's **prepared representation** (GPU buffer/texture, decoded block, atlas). | Matches existing call sites (`mesh->get_rid()`); no separate "loader handle" ambiguity because the loader's RID is owned by `ResourceLoader`. |
| D3 | `Shader` owns a renderer-created program RID; `material_create` takes that RID. | Matches the half-finished `ResSignatureKey { RID shader_rid; uint16_t set; }` in `render_storage.h`. Keeps reflection data on the runtime side. |
| D4 | Materialization is a **pull**: `prepare()` is called by the consumer at first use. `LoadFlags::PREPARE` opts into load-time preparation. | Only attached resources cost VRAM; hot reload and device reset share one generation mechanism. |
| D5 | Neutral vocabulary in `IResource`: `prepare` / `release` / `get_rid` / `get_revision`. No "GPU", no "bound". | The base class must not exclude future non-graphics resources. |
| D6 | Persisted resource references stay `REFLECT Ref<T> { RefCounted* obj; }`; `RefClass<T>` makes them a first-class RTTI type. Archives write a tagged identity (`path` preferred, loader `RID` fallback), never pointer bytes. | Keeps one type for fields, `TypedVar`, and code. `Ref<T>` is packable and reflectable once `RefClass` is registered; no parallel `AssetRef` shim, no conversion layer. No `Ref<T>` field is ever silently dropped. |
| D7 | `ResourceLoader::add()` and the whole "procedural resource in the loader" concept are gone. The editor's procedural generator registry is deleted with it. | A resource is either deserialized from disk (loader-owned) or constructed in memory (caller-owned). Both prepare identically. |

## 6. Current state — inventory

The working tree is a half-finished rewrite. Every item below is a concrete compile or
correctness failure that the phases in §8 close out.

### 6.1 Deleted headers still referenced

`ncore/include/ncore/resources/resource.h` and `.../resource_archive.h` were deleted; the
archive now lives at `ncore/include/ncore/runtime/resources/resource_archive.h`.

Stale `#include` sites:

| File | Line | Stale include |
|---|---|---|
| `ncore/include/ncore/runtime/components.h` | 11 | `<ncore/resources/resource.h>` |
| `ncore/include/ncore/runtime/resources/resource_archive.h` | 5 | `"resource.h"` |
| `ncore/include/ncore/runtime/resources/resource_handle.h` | 3 | `<ncore/resources/resource.h>` |
| `ncore/src/runtime/scene_plugins.cpp` | 4 | `<ncore/resources/resource.h>` |
| `ncore/src/runtime/resources/resource_archive.cpp` | 6 | `<ncore/resources/resource_archive.h>` |
| `ncore/src/services/video/render_service.cpp` | 14 | `<ncore/resources/resource.h>` |
| `ncore/src/services/video/window_service.cpp` | 10 | `<ncore/resources/resource.h>` |
| `ncore/src/services/audio/audio_service.cpp` | 2 | `<ncore/resources/resource.h>` |
| `ncore/src/backends/stb/stb_image_compiler.cpp` | 5 | `<ncore/resources/resource_archive.h>` |
| `ncore/src/backends/stb/stb_cubemap_compiler.cpp` | 5-6 | `<ncore/resources/resource.h>`, `<ncore/resources/resource_archive.h>` |
| `ncore/src/backends/truetype/truetype_font_compiler.cpp` | 5 | `<ncore/resources/resource_archive.h>` |
| `ncore/src/backends/sdl/sdl_audio_compiler.cpp` | 6 | `<ncore/resources/resource_archive.h>` |
| `ncore/src/backends/slang/slang_module_compiler.cpp` | 7-8 | both |
| `tools/editor/src/panels/material_inspector.cpp` | 6 | `<ncore/resources/resource.h>` |

### 6.2 Invalid declarations

- `ncore/include/ncore/services/video/render_service.h:111`
  `RID material_create(RenderType p_render_type, );`
- `ncore/include/ncore/services/video/renderer/render_storage.h:142`
  `RID material_create( RenderType p_render_type,  );`

### 6.3 Stale API assumptions

- `ncore/src/runtime/resources/resources.cpp:217-228` — `Material::Material(const Ref<MaterialShader>&)`
  dereferences `bound_scene` in the constructor, before `ResourceLoader` ever assigns it.
- `ncore/src/runtime/resources/resources.cpp:219,242,249` — uses
  `bound_scene->get_app_ctx()->services.resolve<RenderService>()` and calls the RID+name
  `material_set_param` / `material_set_draw_mode`.
- `ncore/src/runtime/resources/resources.cpp` — `Material` still declares
  `DynamicArray<uint8_t> param_staging`, `RenderService* renderer`, `RID gpu_material`.
- `ncore/src/services/video/render_service.cpp:128` — `cubemap.rid` (`IResource::rid` was removed).
- `ncore/src/services/video/render_service.cpp:216,238,245` and
  `ncore/src/services/video/renderer/render_storage.cpp:195,321` — resource-typed parameters.
- `ncore/src/services/video/renderer/render_storage.cpp` — reads `p_shader->fill_mode`,
  `mat->param_staging`, `mat->texture_slots`, `mat->shader`.
- `ncore/include/ncore/runtime/resources/resource_loader.h:52-54` — `find()` doc still says
  "(disk or virtual)".
- `tools/editor/src/io/editor_assets.cpp:268` — `loader.get_virtual_paths()` (removed).
- `tools/editor/src/io/editor_assets.cpp:500,516` — `loader.find(path)` fallback for
  non-regenerable procedurals.
- `tools/editor/src/io/editor_assets.h:60-64` — `AssetGenerateFn` doc says "typically via
  `ResourceLoader::add(res, path)`".
- `eons-game/main.cpp:98,110` — `res_svc.add( mesh, "procedural/meshes/cube" )`.

### 6.4 Format-id mismatches

Actual `get_format_id()` values:

| Type | ID |
|---|---|
| `Shader` | `nshd` |
| `MaterialShader` | `nmts` |
| `Material` | `nmat` |
| `Mesh` | `nmsh` |
| `Image` | `nimg` |
| `CubeMap` | `ncbm` |
| `AudioClip` | `naud` |
| `Font` | `nfnt` |

`ncore/src/runtime/resources/resource_archive.cpp:25-47` maps `"nmat"` → `MaterialShader`
(should be `"nmts"` → `MaterialShader`, `"nmat"` → `Material`), has **no** `Mesh` entry, and
`known_format()` omits `nmsh`/`nmat`-as-Material.

Deserialization also requires default-constructible types; `Material` and `Mesh` currently have
no default constructor.

---

## 7. Target architecture

### 7.1 Service-side descriptor headers

New files, all header-only, all POD + borrowed spans.

#### `ncore/include/ncore/services/video/renderer/shader_program_desc.h`

Moved verbatim from `runtime/resources/resources.h`: `ShaderParamField`, `ShaderParamDesc`,
`ShaderParamLayout`. Renamed: `ShaderDesc` → `ShaderModuleDesc`. Added:

```cpp
struct ShaderProgramDesc {
    StringView debug_name;
    Span<const ShaderModuleDesc> modules;
};
```

`ShaderParamDesc` keeps `fields`, `offset`, `element_stride`, `total_size_bytes`,
`binding_space`, `binding_idx`, `stage_mask`. Nothing in it refers to a resource.

#### `ncore/include/ncore/services/video/renderer/material_desc.h`

```cpp
struct MaterialStateDesc {
    gfx::CullMode cull_mode                     = gfx::CullMode::NONE;
    bool depth_test                             = false;
    bool depth_write                            = false;
    gfx::BlendPreset blend                      = gfx::BlendPreset::ALPHA_BLEND;
    gfx::MultisampleStateDesc multisample       = { 1, 0 };
    gfx::FillMode fill_mode                     = gfx::FillMode::SOLID;
};

struct MaterialCreateDesc {
    RenderType render_type       = RenderType::SPATIAL;
    RID shader                   = RID::INVALID; // GPU shader program (RenderService::shader_program_create)
    uint32_t param_buffer_size   = 0;             // BINDING_SET_MATERIAL block size, 256-aligned
    MaterialStateDesc state;
};
```

#### `ncore/include/ncore/services/video/renderer/mesh_desc.h`

`MeshLayerDesc` moves here (it is already `REFLECT`-annotated; keep that, it is a POD).
`MeshUploadDesc`:

```cpp
struct MeshUploadDesc {
    Span<const std::byte> vertices;
    Span<const uint32_t> indices;
    uint32_t vertex_stride = 0;
    Span<const MeshLayerDesc> layers; // empty => one implicit whole-buffer layer
};
```

#### `ncore/include/ncore/services/video/renderer/texture_desc.h`

```cpp
struct Texture2DDesc {
    uint32_t width            = 0;
    uint32_t height           = 0;
    gfx::TextureFormat format = gfx::TextureFormat::RGBA8_UNORM;
    Span<const std::byte> data;   // may be empty for an uninitialized/updatable texture
    bool updatable            = false;
    StringView debug_name;
};

struct TextureCubeDesc {
    uint32_t face_size        = 0;
    gfx::TextureFormat format = gfx::TextureFormat::RGBA8_UNORM;
    Span<const std::byte> faces;  // 6 * face_size * face_size * 4, +X -X +Y -Y +Z -Z
    StringView debug_name;
};
```

#### `ncore/include/ncore/services/audio/audio_desc.h`

```cpp
struct AudioStreamDesc {
    Span<const std::byte> data;
    int channels        = 0;
    int frequency       = 0;
    int bits_per_sample = 0;
};
```

#### `ncore/include/ncore/services/video/window_service.h`

`IconDesc { int width = 0; int height = 0; Span<const std::byte> rgba; }` replaces both
`Ref<Image>` parameters. Drop the `class Image;` forward declaration.

### 7.2 `RenderService` / `RenderStorage` API

Methods that change (everything else — swapchain, draw submission, spatial/canvas item CRUD,
`scene_data_update`, texture updates — keeps its current signature):

```cpp
// RenderService
RID shader_program_create( const ShaderProgramDesc& p_desc );
RID texture_2d_create( const Texture2DDesc& p_desc, bool p_updatable = false );
RID texture_cube_create( const TextureCubeDesc& p_desc );
RID mesh_create( const MeshUploadDesc& p_desc );
RID material_create( const MaterialCreateDesc& p_desc );
RID resource_set_create( RID p_shader, gfx::BindingSet p_set, Span<const gfx::ResourceMappingEntry> p_resources );
RID compute_pipeline_create( RID p_shader, Span<const RID> p_resource_sets );
void material_set_param( RID p_material, uint32_t p_offset, const void* p_data, size_t p_data_size );
void material_set_texture( RID p_material, RID p_texture, uint32_t p_slot );   // unchanged
void material_set_draw_mode( RID p_material, gfx::FillMode p_mode );           // unchanged
bool material_is_transparent( RID p_material );                                 // unchanged
uint32_t get_device_revision() const;
```

`RenderStorage` internals:

| Member | Change |
|---|---|
| `RIDPool<MaterialData> materials` | drop `Ref<MaterialShader> shader`; add `bool transparent`, `uint32_t param_buffer_size`, `gfx::FillMode fill_mode` |
| `RIDPool<GPUMesh> meshes` | built from `MeshUploadDesc` instead of `Mesh` |
| `ResSignatureKey { RID shader_rid; uint16_t set; }` | finish the migration: hasher + `operator==` key on `shader_rid`/`set`; drop the `const Shader*` code path |
| new `RIDPool<ShaderProgramData> shader_programs` | `{ RID vs, ps, cs; gfx::VertexLayout vertex_layout; gfx::ShaderStage stage_mask; }` |
| `PSOKey get_pso_key_( ... )` | signature becomes `PSOKey get_pso_key_( RID p_shader, const MaterialStateDesc& p_state )`, reading from `shader_programs` |
| `uint32_t device_revision` | bumped in `RenderService::shutdown()` and whenever `set_graphics_api()` is called |
| `shutdown()` | drain `pending_destroys`, then assert every RID pool is empty (ASAN leak reporter) |

`material_set_param` writes at `p_offset` into the material's `param_buffer` and sets
`param_dirty`; it performs **no** name lookup.

### 7.3 Resource model

New file `ncore/include/ncore/runtime/resources/resource_context.h`:

```cpp
namespace nc {

struct ResourceContext {
    ServiceRegistry* services   = nullptr;
    ResourceLoader*  loader     = nullptr;
    StringView       asset_root;
};

/// Build a context from a live scene (implemented in resources.cpp).
ResourceContext make_resource_context( Scene& p_scene );

} // namespace nc
```

`resources.h` — `IResource` becomes:

```cpp
class NCAPI IResource : public RefCounted {
    NCLASS( IResource, RefCounted )

public:
    virtual ResourceFormatID get_format_id() const { return ResourceFormatID(); }
    /// Bumped whenever the archive payload layout changes. v2 persists Ref<T>
    /// fields as path/RID tags instead of raw pointer bytes.
    virtual uint32_t get_version() const { return 2; }
    virtual size_t get_size_bytes() const { return 0; }

    /// Handle of the prepared representation. INVALID when nothing is prepared,
    /// or when the resource has no prepared representation (e.g. AudioClip).
    RID get_rid() const { return prepared_rid; }

    /// Bumped on every (re)creation of the prepared representation.
    uint32_t get_revision() const { return revision; }

    /// Materialize the live representation and flush pending CPU->backend updates.
    /// Idempotent, main-thread only, pulls dependencies first. Safe to call on an
    /// unbound resource: returns RID::INVALID.
    virtual RID prepare( const ResourceContext& p_ctx ) { return prepared_rid; }

    /// Rebuild non-serialized CPU state after deserialization.
    virtual void on_deserialized() {}

    String filepath = "generated";

protected:
    /// Drop the prepared representation through the owning service. Safe to call
    /// from a destructor after the service shut down (see `service_revision`).
    void release();

    RID      prepared_rid = RID::INVALID;
    uint32_t revision     = 0;
};
```

Removed from `IResource`: `bound_scene`, `on_bound`, `on_unbound`, the `friend class
ResourceLoader`. `resources.h` loses its `class Scene;` forward declaration.

**Teardown safety.** Each GPU-backed resource caches the owning service pointer and the
service's `get_device_revision()` observed at prepare time:

```cpp
void Image::release()
{
    if (prepared_rid.is_valid() && renderer && renderer->get_device_revision() == service_revision)
        renderer->destroy_rid( prepared_rid );
    prepared_rid = RID::INVALID;
    renderer     = nullptr;
}
```

### 7.4 Per-resource `prepare()` / `release()`

| Type | `prepare()` | Notes |
|---|---|---|
| `Shader` | `shader_program_create( ShaderProgramDesc{ filepath, get_stages() } )` | Stages already carry bytecode, entrypoint, params, vertex layout, thread counts. |
| `MaterialShader` | inherits `Shader::prepare` | PSO state fields stay on the resource and are read by `Material`. |
| `Material` | resolve `shader` (`REFLECT Ref<MaterialShader>`); `shader->prepare()`; rebuild if `!rid` or the shader revision changed; then upload staged params + bound textures | Rebuild calls `material_create`, then `material_set_param` per dirty range and `material_set_texture` per slot. |
| `Mesh` | `mesh_create( MeshUploadDesc{ desc.vertices, desc.indices, desc.vertex_stride, desc.layers } )` | |
| `Image` | `texture_2d_create( Texture2DDesc{ width, height, pixels, updatable } )` | |
| `CubeMap` | `texture_cube_create( TextureCubeDesc{ face_size, pixels } )` | |
| `AudioClip` | none (base default) | Playback creates a stream: `AudioService::create_stream( AudioStreamDesc )`. |
| `Font` | none (base default) | Opaque TTF/OTF blob consumed by ImGui. |

`Material` details:

- `param_buffer_size` computed from `shader->get_param_set( RenderStorage::BINDING_SET_MATERIAL )`
  as `max(offset + total_size_bytes)` rounded up to 256.
- `name -> offset` lookup built once from `ShaderParamDesc::fields` of that set.
- `set_param(name, data, size)` → look up offset, `memcpy` into `param_staging`, set
  `param_dirty`; log a warning and return on unknown names. No renderer needed.
- `set_draw_mode(mode)` sets `state.fill_mode` and marks the PSO stale.
- Texture slots: `DynamicArray<Ref<Image>>`/`Ref<CubeMap>` up to `MAX_TEXTURE = 12`; on prepare,
  `prepare()` each bound image and `material_set_texture(rid, image->get_rid(), slot)`.

### 7.5 References and serialization

`Ref<T>` is the one handle type for resources: it is what components hold, what
`TypedVar` boxes, and what archives persist. There is no `AssetRef` shim.

**`Ref<T>` resolves the type that `MaterialHolder`/`MeshInstance` used to spell by hand:**

- `Material` keeps `REFLECT Ref<MaterialShader> shader`.
- `MeshDesc` keeps `REFLECT Ref<Material> material`.
- `SpriteComponent::texture` stays `REFLECT Ref<Image> texture`.
- Delete `ncore/include/ncore/runtime/resources/resource_handle.h` and the `Res<T>` type
  (it also includes the deleted `ncore/resources/resource.h`).
- `Material` and `Mesh` gain public default constructors so `ResourceArchive::make_resource`
  can build them before field visitation.

**RTTI support (`types/`, `object/`).** `Ref<T, TAlloc>` is not a record, so it gets a
`TypeInfo` of its own:

- `RefClass<Ref<T>>` is a `TypeInfo` **and** the erased `IRefOps` interface
  (`target_type`, `get`, `get_refcounted`, `dynamic_type`, `reset`, `share`, `create`).
  `TypeKind::REF`; `TypeInfo::as_ref()` returns the ops, `TypeInfo::is_ref()` is
  `kind == REF`. `RefClass::get()/share()/reset()` drive the real `Ref<T>` refcount, and
  `dynamic_type()` routes through `RecordInfo::dynamic_record()`, so a `Ref<Base>` holding
  a `Derived` reports `Derived` (via `NCLASS`'s virtual `get_class_info()`).
- Defaulted constructor of a `TypedVar` owning a `Ref<T>` clones the **handle**, not the
  pointee: copy increments, move steals, reset releases.
- `TypedVar::is_ref()` / `TypedVar::deref()` expose the pointee as a `TypedPtr` carrying the
  dynamic record type.

**Registration is eager and generator-emitted** (the `VectorClass` pattern). `RefClass`
needs `T` complete (`static_assert( std::derived_from<T, RefCounted> )`, `element_type`,
`sizeof`), and `NCLASS` expands inside the class body where `T` is still incomplete. So
`tools/reflection_gen` emits a `RefClass<nc::Ref<T>>` registration into the `.gen.h` for
**every reflected class whose base chain reaches `nc::RefCounted`**, not just classes that
happen to have a `REFLECT Ref<T>` field. `.gen.h` is aggregated into `reflection_impl.cpp`,
which includes the source header first. `TypeDB::find<Ref<T>>()` therefore works for any
boxed handle (e.g. a `Ref<CubeMap>` that only ever appears as a `TypedVar` value).
Field-driven emission is kept for the same reason; the two dedupe on the type id.

**Archive format (`runtime/io/resource_archive.*`).** `RecordVisitor::reference( t, instance )`
is a new hook; `visit_field()` dispatches `TypeKind::REF` to it instead of `primitive()`, so a
handle is never written as raw pointer bytes. The writer emits one tagged value:

```
[ u8 kind ]  kind = 0 null | 1 path | 2 rid
  path: [ u32 length ][ bytes ]   asset-relative path, cross-session
  rid : [ u64 ]                   ResourceLoader handle, process-local fallback
```

- Path wins: it is the only identity that survives a restart. The `"generated"` sentinel and
  empty paths are not external identities and fall through.
- A handle with no path falls back to `ResourceLoader::find_handle( res )` (a new reverse
  lookup over the loader's cache). It only resolves while that loader lives, which is
  exactly the in-memory/procedural case.
- A non-`IResource` `RefCounted` target is not persistable: it is written as `null` with a
  warning. (Nothing in the engine persists one today.)
- The reader resolves `REF_PATH` through `ResourceLoader::load( path )` and `REF_RID`
  through `ResourceLoader::get( rid )`, then `ops->share()` (which type-checks via `is_a`).
  An unresolvable or mismatched dependency clears the slot and warns: one missing link must
  not fail the whole file.
- `ResourceArchive` takes an optional `ResourceLoader*` (`explicit ResourceArchive( loader )`,
  `deserialize( path, loader = nullptr )`). Offline asset compilers pass nothing; only paths
  are written.
- Recursive loads are guarded by a `HashSet<String> loading` in `ResourceLoader` (RAII
  `LoadingScope`); a `Ref<T>` chain that points back into an in-flight load reads back empty
  with a warning instead of recursing.

**Version bump.** `IResource::get_version()` → `2`. The payload layout changed for any record
containing a ref (`MeshDesc::material`), so existing `.bin` files are rejected with
`version 1 unsupported (resource is 2)` until the asset pipeline regenerates them.

### 7.6 `ResourceLoader`

Pure disk cache. Surface after the change:

```cpp
RID load( const String& p_path, bool p_skip_cache = false );
template<class T> Ref<T> load( const String& p_path, bool p_skip_cache = false );
StringView get_path_of( RID p_rid ) const;
RID find( StringView p_path ) const;
RID find_handle( const IResource* p_resource ) const; // reverse lookup for Ref<T> persistence
void unload_resource( RID p_rid );
void unload_all();
Ref<IResource> get( RID p_rid );
size_t get_resource_count() const;
const Event* peek_event() const;
bool poll_event( Event* p_event );
const char* get_asset_root() const;
```

Changes:

- `LoadFlags::PREPARE` (new enum, default `NONE`). When set, `load()` calls
  `resource->prepare( make_resource_context( scene ) )` after caching and before emitting
  `LoadEvent`. `load()` keeps its boolean `p_skip_cache` parameter for source compatibility and
  forwards.
- No `add()`, no `get_virtual_paths()`, no virtual-path branch in `load()`/`find()`.
- `find()` documentation loses "(disk or virtual)".
- New `HashMap<const IResource*, RID> handles` backs `find_handle()`, maintained in
  `load()`/`unload_resource()`/`unload_all()`. `HashSet<String> loading` backs the
  recursive-load guard.
- `load()` passes `this` into `ResourceArchive::deserialize()` so `Ref<T>` fields resolve.
- `load()` continues to call `archive.deserialize()` → which calls
  `resource->on_deserialized()`.

### 7.7 ECS components (`ncore/include/ncore/runtime/components.h`)

- Remove `#include <ncore/resources/resource.h>`.
- Delete `HasResourceTag` and `ResourceLoadedComponent` — they existed only to track loader
  handles.
- Rename `MeshComponent` → `MeshInstanceComponent` and `MaterialComponent` →
  `MaterialOverrideComponent` (they describe an instance/override, not the resource):

```cpp
NC_COMPONENT_API( MeshInstanceComponent )
{
    /// The resource itself. File-backed when loaded through the archive,
    /// constructed in memory otherwise; the Ref<T> keeps it alive either way.
    REFLECT Ref<Mesh> mesh;
    /// Entity-specific: which mesh layer to draw.
    REFLECT uint8_t layer_index = 0;
    /// Entity-specific: instanced draw count.
    REFLECT uint32_t instance_count = 1;
    /// Engine-owned spatial item. Populated by register_video_plugin; hidden
    /// from the inspector and never serialized by an author.
    REFLECT_HINT( types::PropertyFlags::HIDDEN ) RID internal_handle = 0;
};

NC_COMPONENT_API( MaterialOverrideComponent )
{
    REFLECT Ref<Material> material;
    /// Entity-specific: draw with this material instead of the mesh's own.
    REFLECT bool override_mesh_material = false;
};
```

`Transform3DComponent`, `CameraComponent` etc. keep only their entity-specific fields;
`SpriteComponent::texture` stays `REFLECT Ref<Image> texture`.

`Ref<T>` fields are real `TypeKind::REF` properties, so the inspector shows the pointee and
scene/archive persistence writes the handle's identity (see §7.5).

### 7.8 Submit path (`ncore/src/runtime/scene_plugins.cpp`)

`register_video_plugin` currently creates GPU meshes/materials per component and destroys them
on removal. It becomes a pure consumer:

```cpp
auto ctx = make_resource_context( scene );
if (instance->mesh) {
    if (RID rid = instance->mesh->prepare( ctx ); rid.is_valid())
        vid->renderer->spatial_item_set_mesh( instance->internal_handle, rid );
}
```

Cache the last `(Ref<T>*, revision)` per spatial item so the call collapses to a pointer +
integer compare on steady-state frames.

### 7.9 Editor (`tools/editor/src/io/`)

Delete the procedural generator machinery:

- `AssetGenerateFn`, `ProceduralRecord`, `register_procedural()`, `unregister_procedural()`,
  `ensure_procedural_entry()`, the `generators` map, `load_or_generate()`, `AssetOrigin`,
  `asset_origin_name()`.
- `AssetDatabase::scan()` loses the two loops at `editor_assets.cpp:266-308`; it becomes
  file-record reconciliation only.
- `AssetDatabase::ensure()` → `loader.load( path )`.

Call-site fallout:

| File | Change |
|---|---|
| `src/panels/assets_panel.cpp:88,222,309,446,449` | drop `AssetOrigin` checks; `load_or_generate` → `ensure` |
| `src/panels/inspector_panel.cpp:190-191` | drop the procedural branch; `load_or_generate` → `ensure` |
| `src/panels/asset_picker.cpp` | pick a file, then assign `loader.load<Mesh>( path )` / `load<Material>( path )` into the component's `Ref<T>` field |
| `src/io/thumbnail_cache.cpp` | loader path only; key on the asset path, value on `get_rid()` |
| `src/panels/material_inspector.cpp` | rebind to `MaterialOverrideComponent { Ref<Material> material; }` instead of the deleted RID-based component |
| `tools/editor/AGENTS.md` | "asset pipeline" map loses the procedural registry; smoke expectation `18 assets` → `16 assets` |

### 7.10 Window / Audio

- `WindowService::set_default_icon( const IconDesc& )`,
  `window_set_icon( uint32_t, const IconDesc& )`. `default_app_icon` becomes a small owned
  `DynamicArray<std::byte>` + dimensions.
- `AudioService::create_stream( const AudioStreamDesc& )`; `audio_service.cpp` drops the
  `resource.h` include and reads the span instead of `AudioClip` accessors.
- `AudioClip` does **not** get a prepared representation.

---

## 8. Phases

Each phase lists its goal, the files it touches, the work, and the acceptance check. Note that
the tree is expected to stay red for `ncore` until phase 3 and for the app targets until phase
6 — the phases are ordered by module, not by "green at every step".

### Phase 0 — Mechanical repair (forward-only)

**Goal.** Remove the noise so every remaining error is one the plan addresses.

**Files.** `ncore/include/ncore/runtime/components.h`, `.../runtime/resources/resource_archive.h`,
`.../runtime/resources/resource_handle.h`, `ncore/src/runtime/resources/resource_archive.cpp`,
`ncore/src/services/video/render_service.cpp`, `.../window_service.cpp`, `.../audio_service.cpp`,
`ncore/src/backends/{stb,truetype,sdl,slang}/*.cpp`, `ncore/src/runtime/scene_plugins.cpp`,
`tools/editor/src/panels/material_inspector.cpp`.

**Work.**

1. Repoint every stale `ncore/resources/...` include per §6.1 at
   `ncore/runtime/resources/...`.
2. Delete `resource_handle.h` and the `Res<T>` type (D7); fix or remove its users.
3. Replace the two `material_create(RenderType,)` stubs with
   `RID material_create( const MaterialCreateDesc& )` and leave a `TODO(phase-2)` body.
4. Delete `editor_assets.cpp:266-283` (`get_virtual_paths`) and the procedural branches, so the
   editor compiles against the D7 API.

**Done when.** Building `ncore_d` reports no "file not found" or "not declared" errors; remaining
errors are confined to `render_storage.*`, `render_service.*`, `resources.cpp` and the phase-6
consumer set.

### Phase 1 — Service descriptors

**Goal.** Every neutral descriptor exists on the service side before anything consumes it.

**Files.** New `services/video/renderer/{shader_program_desc,material_desc,mesh_desc,texture_desc}.h`,
`services/audio/audio_desc.h`; `ncore/include/CMakeLists.txt`.

**Work.**

1. Move `ShaderParamField`, `ShaderParamDesc`, `ShaderParamLayout`, `MeshLayerDesc` out of
   `runtime/resources/resources.h`; re-include them there.
2. Rename `ShaderDesc` → `ShaderModuleDesc`; add `ShaderProgramDesc`.
3. Add `MaterialStateDesc`, `MaterialCreateDesc`, `MeshUploadDesc`, `Texture2DDesc`,
   `TextureCubeDesc`, `AudioStreamDesc`.
4. Register the new headers in `ncore/include/CMakeLists.txt`.

**Done when.** `runtime/resources/resources.h` compiles with only service includes for gfx types
and descriptors; no duplicate definitions.

### Phase 2 — `RenderStorage`

**Goal.** The renderer holds no resource types and no reflection data.

**Files.** `ncore/include/ncore/services/video/renderer/render_storage.h`,
`ncore/src/services/video/renderer/render_storage.cpp`.

**Work.**

1. Add `RIDPool<ShaderProgramData> shader_programs` + `shader_program_create/destroy`.
2. `material_create( const MaterialCreateDesc& )`; store `transparent`, `param_buffer_size`,
   `fill_mode`; remove `Ref<MaterialShader> shader`.
3. `material_set_param( RID, uint32_t offset, const void*, size_t )` — offset write only.
4. `resource_set_create( RID shader, ... )` reading stages/layouts from `shader_programs`.
5. `get_pso_key_( RID shader, const MaterialStateDesc& )`.
6. `mesh_create( const MeshUploadDesc& )`.
7. Finish `ResSignatureKey { RID shader_rid; uint16_t set; }` (hasher + equality).
8. Add `device_revision`, bump it in `shutdown()` / `set_graphics_api()`, and drain pools.
9. Delete every `#include` of `runtime/**` from the header and source.

**Done when.** `render_storage.h` includes only `core/**` and `services/**`; `render_storage.cpp`
compiles standalone; PSO caching still returns the same permutation for identical
`(shader, state)`.

### Phase 3 — `RenderService` facade and engine call sites

**Goal.** `ncore` builds again.

**Files.** `ncore/include/ncore/services/video/render_service.h`,
`ncore/src/services/video/render_service.cpp`, `seaengine/src/water/wave_generator.cpp`,
`seaengine/src/tests/test_compute_shader.cpp`.

**Work.**

1. Replace the six resource-typed methods with the descriptors from §7.2.
2. Add `shader_program_create()` passthrough and `get_device_revision()`.
3. Fix `cubemap.rid` → descriptor-built `TextureCubeDesc`.
4. Update `wave_generator.cpp`: `RID shader = shader_res->prepare( ctx )`, then
   `resource_set_create( shader, set, ... )` and `compute_pipeline_create( shader, sets )`.
5. Same for `test_compute_shader.cpp`.

**Done when.** `ninja -C build/windows-debug ncore_d.dll` is green.

### Phase 4 — Resource model

**Goal.** `IResource` drives services; nothing in the resource layer needs a `Scene`.

**Files.** `ncore/include/ncore/runtime/resources/resources.h`,
`ncore/src/runtime/resources/resources.cpp`, new `runtime/resources/resource_context.h`,
`ncore/include/CMakeLists.txt`, `ncore/src/CMakeLists.txt`.

**Work.**

1. Replace `IResource` per §7.3 (drop `bound_scene`, `on_bound`, `on_unbound`, `friend`).
2. Add `ResourceContext` + `make_resource_context( Scene& )`.
3. Keep `Material::shader` and `MeshDesc::material` as `REFLECT Ref<T>`; no `AssetRef`.
4. Implement `prepare()`/`release()` per §7.4, including the service-revision guard.
5. Give `Material` and `Mesh` default constructors.
6. Implement `Material::set_param` as CPU staging (`param_staging`, `param_dirty`,
   `name -> offset`), and `set_draw_mode` as a state change + PSO-stale flag.
   The texture branch accepts either a boxed `RID` or a `TypedVar` holding a `Ref<T>`
   (`is_ref()` → `deref()` → `try_as<IResource>()` → `get_rid()`).
7. Reimplement `Material::Material(const Ref<MaterialShader>&)` as an in-memory convenience
   constructor that assigns `shader` directly (no service access).

**Done when.** `ncore_d` still builds and `resources.cpp` contains no `#include <ncore/runtime/scene.h>`.
`grep` for `bound_scene` returns nothing.

### Phase 5 — Loader and archive

**Goal.** One disk cache; every format round-trips.

**Files.** `ncore/include/ncore/runtime/resources/resource_loader.h`,
`ncore/src/runtime/resources/resource_loader.cpp`,
`ncore/include/ncore/runtime/resources/resource_archive.h`,
`ncore/src/runtime/resources/resource_archive.cpp`,
`ncore/include/ncore/types/{types,typeinfo,classinfo}.h`, `ncore/include/ncore/object/classinfo.h`,
`ncore/include/ncore/types/variant.h`, `ncore/src/types/typeinfo.cpp`,
`tools/reflection_gen/ast_visitor.cpp`.

**Work.**

1. Delete the virtual-path branches and the `add()` remnants; update `find()` docs.
2. Add `LoadFlags::PREPARE` and wire it through both `load()` overloads.
3. Fix `make_resource()`: `nshd`→`Shader`, `nmts`→`MaterialShader`, `nmat`→`Material`,
   `nmsh`→`Mesh`, `nimg`→`Image`, `ncbm`→`CubeMap`, `naud`→`AudioClip`, `nfnt`→`Font`.
4. Fix `known_format()` and `format_id_string()` coverage to match.
5. `Ref<T>` RTTI per §7.5: `RefClass`/`IRefOps`, `TypeInfo::is_ref()/as_ref()`,
   virtual `RecordInfo::dynamic_record()/create_refcounted()`, `RecordVisitor::reference()`,
   `TypedVar::is_ref()/deref()`, and the generator's eager `RefClass` emission.
6. Archive tagged ref encoding + `ResourceLoader::find_handle()` + the `loading` cycle guard,
   and pass the loader through `deserialize()`.
7. Bump `IResource::get_version()` to `2`.
8. Confirm `deserialize()` calls `on_deserialized()` and nothing else.

**Done when.** Every `.bin` produced by the asset pipeline deserializes to its concrete type; a
forced reload emits `LoadEvent { handle, format_id, replaced }` with `replaced` set; a
`Ref<T>` field round-trips through path and through RID fallback.

### Phase 6 — Consumers: ECS, game, editor

**Goal.** All four targets build.

**Files.** `ncore/include/ncore/runtime/components.h`,
`ncore/src/runtime/scene_plugins.cpp`, `eons-game/main.cpp`,
`tools/editor/src/{io,panels}/*`, `tools/editor/AGENTS.md`.

**Work.**

1. Components per §7.7; delete `HasResourceTag` / `ResourceLoadedComponent`.
2. `register_video_plugin` becomes a pure consumer per §7.8 (both mesh and material passes).
3. `eons-game/main.cpp`: `Ref<Mesh>::create(MeshDesc{...})`, `Ref<MaterialShader>::create(...)` /
   `res_svc.load<MaterialShader>(...)`, `Ref<Material>::create(shader)`, assign into components;
   no `res_svc.add`.
4. Editor per §7.9.
5. Update `tools/editor/AGENTS.md` (procedural registry removed; smoke count 16).

**Done when.** `ninja -C build/windows-debug ncore_d.dll ncore_editor_d.dll EonsPrototype_d.exe SeaEngine_d.exe`
is green.

### Phase 7 — Window, audio, audit

**Files.** `services/video/window_service.h`, `src/services/video/window_service.cpp`,
`services/audio/audio_service.h`, `src/services/audio/audio_service.cpp`,
`services/io/asset_compiler.h` (comment only).

**Work.**

1. `IconDesc` per §7.10; `WindowService` no longer mentions `Image`.
2. `AudioStreamDesc`; drop the `resource.h` include.
3. Add the exemption comment block to `asset_compiler.h`.
4. Run the audit grep from §4.

**Done when.** The audit returns only `asset_compiler.h`, and `services/**` has no other
`runtime/` include.

### Phase 8 — Build and smoke

1. Rebuild assets (`asset_pipeline`) — the format-id fixes invalidate existing `.bin` files.
2. Build all four targets with the x64 `LIB` override.
3. Smoke run per §10.

---

## 9. Risks

| Risk | Mitigation |
|---|---|
| **Asset invalidation.** The version bump (`get_version()` → 2) makes existing `.bin` files unreadable, as do the format-id fixes. | Rebuild the asset pipeline in phase 8 before smoke testing, deleting stale `.bin`s (the pipeline only recompiles when the source timestamp changes); keep the `--search-paths` behaviour intact. |
| **Teardown order.** Resources can outlive `RenderService::shutdown()`. | Service-revision guard in `release()`; drain + assert empty pools in `shutdown()`; watch the ASAN leak reporter. |
| **`Ref<T>` completeness.** `RefClass<Ref<T>>` and `Ref<T>`'s destructor need a complete `T`. | Registration is emitted into `.gen.h` (aggregated after the source header), never from `NCLASS` inside the class body. `ObjectClass`'s virtual hooks are gated on `types::detail::is_complete_v` so plain `NSTRUCT_V` structs still instantiate. |
| **Non-`IResource` refs are not persisted.** A `Ref<MyRefCounted>` field writes `null` plus a warning. | Deliberate: only assets have a stable identity. If non-resource refs must persist, they need their own identity scheme, not a pointer write. |
| **RID fallback is process-local.** A `REF_RID` entry only resolves while the same loader is alive. | Paths are preferred; the RID branch exists solely for in-memory resources in one session. The reader warns and clears the slot when it cannot resolve. |
| **Recursive ref chains.** `A` referencing `B` referencing `A` would recurse through `load()`. | `ResourceLoader::loading` cycle guard; the dependent ref reads back empty with a warning instead of blowing the stack. |
| **`-Werror` family.** | `-Wshadow` on `p_` names; `[[clang::lifetimebound]]` on stored-reference params; no header-level function-local statics (`-Wexit-time-destructors`). |
| **Primitive sharing.** The loader no longer dedupes in-memory resources. | A per-scene `Ref<CubeMesh>`/`Ref<PlaneMesh>` holder (or assigning the same `Ref<T>` into both components) when two nodes must share one GPU mesh. |
| **Inspector shows refs read-only.** `Ref<T>` is now a real property (pointee name/id), but there is no inline asset browser on it. | Assign through the `AssetPicker` flow (`loader.load<T>( path )` → component field). Editable-path `Ref` properties are a follow-up, not a blocker. |

## 10. Verification

Build (run from repo root, with the x64 `LIB` override from `tools/editor/AGENTS.md`):

```powershell
ninja -C build/windows-debug ncore_d.dll ncore_editor_d.dll EonsPrototype_d.exe SeaEngine_d.exe
```

Smoke test:

```powershell
cd build\windows-debug\bin\Debug
$err = "$env:TEMP\smoke.txt"; Remove-Item $err -ErrorAction SilentlyContinue
$p = Start-Process -FilePath ".\EonsPrototype_d.exe" -PassThru -RedirectStandardError $err
Start-Sleep -Seconds 40
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; "alive" } else { "exited 0x{0:X8}" -f $p.ExitCode }
Select-String -Path $err -Pattern "FATAL|Assertion|AddressSanitizer"
Select-String -Path logs\engine.log -Pattern "AssetDatabase:" | Select-Object -Last 1
```

Pass criteria:

- Process alive; no `AddressSanitizer` / assertion in stderr.
- `AssetDatabase: 16 assets` (was 18 — the two procedural entries are gone by design).
- Main menu visible; scene tree populated; inspector shows component data; camera and stats
  windows toggle.
- Cube and plane still render (they now go through `Ref<Mesh>` + `prepare()`).
- Toggling wireframe in the toolbar still switches the PSO (exercises `material_set_draw_mode`).
- Forced reload of `standard_spatial.slang` does not crash and the material rebuilds (exercises
  the revision cascade).

## 11. Follow-ups (not in this plan)

- Async loading; `ResourceLoader::get()`'s "TODO: justified once asynchronous" comment becomes
  actionable.
- Scene serialization: components already carry `Ref<T>`, and the archive already persists a ref's
  path/RID, so only the authoring path is new.
- Re-introduce a *library* (not a loader) for shared built-in primitives if the per-scene holder
  proves insufficient.
- Device-lost handling beyond the revision guard (currently: everything rebuilds on the next
  prepare, which is correct but not incremental).

## 12. Godot research follow-up candidates

Source: `.scratch/resource-service-inversion/godot-resource-model.md` (research notes against
Godot master). Only items that map onto *our* model are listed; each cites the Godot symbol the
finding comes from.

| # | Candidate | Godot evidence | Why it matters for us |
|---|---|---|---|
| G1 | **Placeholder texture instead of skipping invalid slots.** When a resource isn't materialized, Godot returns a placeholder RID (`CompressedTexture2D::get_rid()` → `texture_2d_placeholder_create`). We currently skip invalid slots at `prepare()` time; a placeholder would keep uniform sets stable. | `scene/resources/compressed_texture.cpp` — `CompressedTexture2D::get_rid()` | Material uniform sets (already created from `MaterialCreateDesc` reflection) don't need re-binding when a texture slot is temporarily empty. |
| G2 | **Clear texture RID arrays on material release.** Godot's `MaterialStorage::material_free` clears texture arrays before freeing the material "to prevent spin locking of their RIDs". Our `Material::release()` should clear `texture_rids` / `textures` so direct `RID` bindings don't dangle into a re-prepare. | `servers/rendering/renderer_rd/storage_rd/material_storage.cpp` — `MaterialStorage::material_free` | Our new `texture_rids` array holds caller-owned RIDs; if they survive `release()`, the next `prepare()` re-binds stale handles. |
| G3 | **Reject object params in `set_param`**. Godot disallows `Variant::OBJECT` in `material_set_param` (`ERR_FAIL_COND`) — resources must go through the named-texture path. Our `Material::set_param` takes a raw `void*`+size with no type check. | `servers/rendering/renderer_rd/storage_rd/material_storage.cpp` — `MaterialStorage::material_set_param` | Keeps the "name → RID" texture path authoritative; a stray `Ref<T>` handle magically written into the param block would silently corrupt. |
| G4 | **std140-computed param sizes as the escape hatch.** Godot computes UBO layout per-shader (`_fill_std140_variant_ubo_value`), not from a fixed 256-byte staging block. Our fixed block is simpler and faster; if a shader's params exceed it or reflection drifts, std140-computed sizing is the documented alternative. | `servers/rendering/renderer_rd/storage_rd/material_storage.cpp` — `MaterialData::update_uniform_buffer` | Confirms our `param_buffer_size` (256-aligned from reflection) is a deliberate simplification, not an oversight — worth a comment in `rebuild_layout_()`. |
| G5 | **Queued, frame-boundary invalidation.** Godot queues material updates (`_material_queue_update`) and drains them at a frame boundary; invalidation is never immediate. Our `prepare()` rebuilds immediately. | `servers/rendering/renderer_rd/storage_rd/material_storage.cpp` — `_material_queue_update` / `_update_queued_materials` | Relevant if per-frame shader hot-reloads cause redundant rebuilds; batching `prepare()` work into a drain phase matches our existing `pump()` pattern in the importer. |
| G6 | **Server-side dependency tracker for uniform-set loss.** Godot re-creates uniform sets when a texture they reference is deleted (`_material_uniform_set_erased`). Our model has no equivalent — deleting a texture RID referenced by a prepared material is currently silent corruption. | `servers/rendering/renderer_rd/storage_rd/material_storage.cpp` — `MaterialStorage::_material_uniform_set_erased` | Either (a) document that direct `RID` texture bindings are caller-owned and must be re-set after release, or (b) add a dependency hook in `RenderStorage`. |

Non-items (researched, deliberately *not* adopted):

- **HashMap + dirty-flag param staging** — our fixed staging buffer is simpler; G4 documents the
  trade-off.
- **Godot's "no neutral descriptor" layer** — Godot passes raw `RID`/`Variant` into the renderer.
  Our neutral `MaterialCreateDesc` layer exists precisely to keep `IResource` portable; don't
  remove it.
- **`emit_changed()` signal network** — Godot invalidates via resource signals + server
  dependency tracker. Our revision-guard model (`get_revision()` + service revision check in
  `prepare()`) is the single mechanism the spec mandates (§2); introducing an observer network
  would contradict it.
- **Hybrid eager/lazy materialization** — Godot is eager for `ImageTexture::set_image()` and lazy
  for `Material`. We stay uniformly lazy (D4); eager upload would need a push-style API that
  contradicts the pull model.
