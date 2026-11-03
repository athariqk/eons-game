# Godot Resource Model — Research Notes

## TL;DR Comparison with our model

**Godot (master):**
- Resources own `RID`s directly (e.g. `Material`, `Shader`, `ImageTexture`).
- Materialization is *lazy* for `Material`/`Shader` (the RID is empty until `get_rid()` / `_check_material_rid()` is called) and *eager* for `ImageTexture` (created immediately on `set_image()`).
- `MaterialStorage` (server side) owns the GPU state and stages params in `material->params` (uniform/texture dirty flags).
- Textures are resolved by *name* (`MaterialData::update_textures`) through `TextureStorage`, not by *slot* or *descriptor*.

**Our model (IResource / ResourceContext):**
- `IResource` with `prepare()/release()/get_rid()/get_revision()` — conceptually similar to `_check_material_rid()` + `get_rid()` but the Godot model is driven by RefCounted resource lifetime.
- Pull-based materialization is similar (`ShaderMaterial::ShaderMaterial()` is empty until `get_rid()`), but Godot has *no* neutral descriptor layer; the renderer takes `RID` directly, and `MaterialStorage::material_set_param` takes `Variant` (not a neutral descriptor).
- No 256-byte-aligned staging; Godot writes params into `std140`-computed UBO (`MaterialData::update_uniform_buffer`), not a fixed-size staging buffer.

---

## 1. Resource ownership & RID

In Godot, GPU resources are identified by `RID` (a `uint64_t`-like handle). `Resource` is `RefCounted`:

```cpp
// core/io/resource.h
class Resource : public RefCounted {
	...
	virtual RID get_rid() const; // Some resources may offer conversion to RID.
};
```

`Material` owns the RID directly in `scene/resources/material.h`:

```cpp
class Material : public Resource {
	mutable RID material;
	Ref<Material> next_pass;
	int render_priority;
	enum { INIT_STATE_UNINITIALIZED, INIT_STATE_INITIALIZING, INIT_STATE_READY } init_state;
protected:
	_FORCE_INLINE_ void _set_material(RID p_material) const { material = p_material; }
	_FORCE_INLINE_ RID _get_material() const { return material; }
```

- `Material::get_rid()` returns `material` (`scene/resources/material.cpp` lines 88-90):
  ```cpp
  RID Material::get_rid() const { return material; }
  ```
- `Material::~Material()` frees the RID (`scene/resources/material.cpp` lines 191-195):
  ```cpp
  Material::~Material() {
      if (material.is_valid()) {
          ERR_FAIL_NULL(RenderingServer::get_singleton());
          RenderingServer::get_singleton()->free_rid(material);
      }
  }
  ```
- `Material::set_next_pass` (in `scene/resources/material.cpp`) immediately pushes the RID change if the RID is valid:
  ```cpp
  if (material.is_valid()) {
      RID next_pass_rid;
      if (next_pass.is_valid()) {
          next_pass_rid = next_pass->get_rid();
      }
      RS::get_singleton()->material_set_next_pass(material, next_pass_rid);
  }
  ```
- `Resource::emit_changed()` emits the `changed` signal (`core/io/resource.cpp`), but Godot does *not* have a generic "RID_INVALIDATE" hook on the resource side; instead, invalidation is driven by the server side (`MaterialStorage::material_set_param` queues updates, `Shader::set_code` pushes to `shader_set_code`, and `Material::_validate_property` only alters property metadata for the editor).

`Resource::emit_changed()` respects blocked state during loading/duplication:
```cpp
void Resource::emit_changed() {
    if (emit_changed_state != EMIT_CHANGED_UNBLOCKED) {
        emit_changed_state = EMIT_CHANGED_BLOCKED_PENDING_EMIT;
        return;
    }
    if (ResourceLoader::is_within_load() && !Thread::is_main_thread()) {
        ResourceLoader::resource_changed_emit(this);
        return;
    }
    emit_signal(CoreStringName(changed));
}
```

---

## 2. Lazy/pull-based materialization vs eager

Godot mixes both models.

### Lazy (materials & shaders)
`ShaderMaterial` does not allocate its RID at construction (`scene/resources/material.cpp` lines 548-550):
```cpp
ShaderMaterial::ShaderMaterial() {
    // Material RID will be empty until it is required.
}
```
RID creation is deferred to `get_rid()` / `_check_material_rid()` (`scene/resources/material.cpp` lines 468-492, 535-537):
```cpp
RID ShaderMaterial::get_rid() const {
    _check_material_rid();
    return Material::get_rid();
}

void ShaderMaterial::_check_material_rid() const {
    MutexLock lock(material_rid_mutex);
    if (_get_material().is_null()) {
        RID shader_rid = shader.is_valid() ? shader->get_rid() : RID();
        ...
        _set_material(RS::get_singleton()->material_create_from_shader(next_pass_rid, get_render_priority(), shader_rid));
        for (KeyValue<StringName, Variant> param : param_cache) {
            ...RS::get_singleton()->material_set_param(...);
        }
    }
}
```

`Shader` follows the same pattern:
```cpp
Shader::Shader() {
    // Shader RID will be empty until it is required.
}

void Shader::_check_shader_rid() const {
    MutexLock lock(shader_rid_mutex);
    if (shader_rid.is_null() && !preprocessed_code.is_empty()) {
        shader_rid = RenderingServer::get_singleton()->shader_create_from_code(preprocessed_code, get_path());
        preprocessed_code = String();
    }
}
```

`BaseMaterial3D` also has a lazy RID (`_check_material_rid`, lines 2116-2132) but stages params in `pending_params` when the RID isn't yet created (`_material_set_param`, lines 2162-2168):
```cpp
void BaseMaterial3D::_material_set_param(const StringName &p_name, const Variant &p_value) {
    if (_get_material().is_valid()) {
        RS::get_singleton()->material_set_param(_get_material(), p_name, p_value);
    } else {
        pending_params[p_name] = p_value;
    }
}
```

### Eager (textures on `set_image`)
`ImageTexture` creates the GPU texture eagerly on `set_image`:
```cpp
void ImageTexture::set_image(const Ref<Image> &p_image) {
    ...
    if (texture.is_null()) {
        texture = RenderingServer::get_singleton()->texture_2d_create(p_image);
    } else {
        RID new_texture = RenderingServer::get_singleton()->texture_2d_create(p_image);
        RenderingServer::get_singleton()->texture_replace(texture, new_texture);
    }
    emit_changed();
}
```

### Placeholder on first use (CompressedTexture2D)
`CompressedTexture2D::get_rid()` (in `scene/resources/compressed_texture.cpp`) returns a placeholder if not loaded:
```cpp
RID CompressedTexture2D::get_rid() const { ... }
```
(Note: `ImageTexture::get_rid()` also does a `texture_2d_placeholder_create()` fallback if empty).

**Verdict:** Godot is pull-based for `Material` and `Shader` (lazy materialization at `get_rid()`), but *eager* for `ImageTexture::set_image` and *placeholder-based* for `CompressedTexture2D::get_rid()` — it is not strictly "pull-based first-use materialization" in our model's sense; it's a hybrid.

---

## 3. Server-object lifecycle

Server-side objects (materials, shaders, textures) are owned by `MaterialStorage` / `TextureStorage` and freed by `RenderingServer::free_rid()`:

```cpp
void MaterialStorage::material_free(RID p_rid) {
    Material *material = material_owner.get_or_null(p_rid);
    ERR_FAIL_NULL(material);
    // Need to clear texture arrays to prevent spin locking of their RID's.
    for (KeyValue<StringName, Variant> &E : material->params) {
        if (E.value.get_type() == Variant::ARRAY) {
            E.value = Variant();
        }
    }
    material_set_shader(p_rid, RID()); //clean up shader
    material->dependency.deleted_notify(p_rid);
    material_owner.free(p_rid);
}
```

`MaterialStorage::material_set_shader` (line 2515) invalidates the shader data and requests a new `MaterialData` based on shader type when the shader changes (`material_data_request_func[shader->type]`):
```cpp
void MaterialStorage::material_set_shader(RID p_material, RID p_shader) {
    Material *material = material_owner.get_or_null(p_material);
    ...
    if (material->data) {
        memdelete(material->data);
        material->data = nullptr;
    }
    ...
    material->data = material_data_request_func[shader->type](shader->data);
    ...
    _material_queue_update(material, true, true);
}
```

If a Resource dies, it explicitly frees the RID in its destructor (e.g. `ImageTexture::~ImageTexture()`, `Material::~Material()`, `Shader::~Shader()`), so there is no dangling-RID walk in `RenderingServer::~RenderingServer()` (which just sets `singleton = nullptr`, `servers/rendering_server.cpp` lines 3851-3853):
```cpp
RenderingServer::~RenderingServer() {
    singleton = nullptr;
}
```
`RenderingServer::_free_internal_rids()` only frees the internal test/white textures/materials:
```cpp
void RenderingServer::_free_internal_rids() {
    if (test_texture.is_valid()) { free_rid(test_texture); }
    ...
}
```
`ResourceCache::clear()` warns if resources remain in use at exit:
```cpp
void ResourceCache::clear() {
    if (!resources.is_empty()) {
        ERR_PRINT(vformat("%d resources still in use at exit.", resources.size()));
        ...
    }
    resources.clear();
}
```

---

## 4. Staging/dirty-tracking of parameters

Server-side `Material` has a `params` HashMap and dirty flags (`uniform_dirty`, `texture_dirty`):
```cpp
void MaterialStorage::_material_queue_update(Material *material, bool p_uniform, bool p_texture) {
    MutexLock lock(material_update_list_mutex);
    material->uniform_dirty = material->uniform_dirty || p_uniform;
    material->texture_dirty = material->texture_dirty || p_texture;
    if (material->update_element.in_list()) {
        return;
    }
    material_update_list.add(&material->update_element);
}
```

`MaterialStorage::material_set_param` (line 2574) pushes params into `material->params` and marks dirty flags (`material->uniform_dirty` / `material->texture_dirty`):
```cpp
void MaterialStorage::material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) {
    Material *material = material_owner.get_or_null(p_material);
    ERR_FAIL_NULL(material);
    if (p_value.get_type() == Variant::NIL) {
        material->params.erase(p_param);
    } else {
        ERR_FAIL_COND(p_value.get_type() == Variant::OBJECT); //object not allowed
        material->params[p_param] = p_value;
    }
    if (material->shader && material->shader->data) {
        bool is_texture = material->shader->data->is_parameter_texture(p_param);
        _material_queue_update(material, !is_texture, is_texture);
    }
}
```

`MaterialStorage::_update_queued_materials` (line 2457) drains the queue and calls `MaterialData::update_parameters`:
```cpp
void MaterialStorage::_update_queued_materials() {
    ...
    while (SelfList<Material> *E = material_update_list.first()) {
        ...
        Material *material = E->self();
        if (material->data) {
            material->data->update_parameters(material->params, material->uniform_dirty, material->texture_dirty);
        }
        material->texture_dirty = false;
        material->uniform_dirty = false;
    }
}
```

`MaterialData::update_uniform_buffer` (line 747) is the actual writer to the UBO (not a fixed staging buffer — it uses a dynamically-sized `std140`-layout buffer `ubo_data`).
`MaterialData::update_textures` (line 881) resolves texture uniform name -> RID into `texture_cache` and builds the uniform set (`uniform_set`, bound via `RD::UNIFORM_TYPE_TEXTURE`).

`BaseMaterial3D` on the scene side also has a dirty list (`dirty_materials`) and `_queue_shader_change()` / `flush_changes()`:
```cpp
SelfList<BaseMaterial3D>::List BaseMaterial3D::dirty_materials;
void BaseMaterial3D::_queue_shader_change() {
    if (!_is_initialized()) {
        return;
    }
    MutexLock lock(material_mutex);
    if (!element.in_list()) {
        dirty_materials.add(&element);
    }
}
```

**Verdict:** Godot *does* use staging (`material->params` HashMap) + dirty flags (`uniform_dirty` / `texture_dirty`) but there is no 256-byte-aligned staging buffer; the UBO size is computed from the shader's uniforms (`_fill_std140_variant_ubo_value`).

---

## 5. Texture binding model

`ShaderMaterial` binds textures by *uniform name* via `set_shader_parameter` (`scene/resources/material.cpp`):
```cpp
void ShaderMaterial::set_shader_parameter(const StringName &p_param, const Variant &p_value) {
    RID material_rid = _get_material();
    if (p_value.get_type() == Variant::NIL) {
        param_cache.erase(p_param);
        if (material_rid.is_valid()) {
            RS::get_singleton()->material_set_param(material_rid, p_param, Variant());
        }
    } else {
        Variant *v = param_cache.getptr(p_param);
        if (!v) {
            remap_cache["shader_parameter/" + p_param.string()] = p_param;
            param_cache.insert(p_param, p_value);
        } else {
            *v = p_value;
        }
        if (p_value.get_type() == Variant::OBJECT) {
            RID tex_rid = p_value;
            if (tex_rid == RID()) {
                param_cache.erase(p_param);
                if (material_rid.is_valid()) {
                    RS::get_singleton()->material_set_param(material_rid, p_param, Variant());
                }
            } else if (material_rid.is_valid()) {
                RS::get_singleton()->material_set_param(material_rid, p_param, tex_rid);
            }
        } else if (material_rid.is_valid()) {
            RS::get_singleton()->material_set_param(material_rid, p_param, p_value);
        }
    }
}
```

`MaterialData::update_textures` (server-side) resolves the texture uniform name -> RID (`TextureStorage::get_singleton()->get_texture(textures[j])`):
```cpp
void MaterialStorage::MaterialData::update_textures(const HashMap<StringName, Variant> &p_parameters, const HashMap<StringName, HashMap<int, RID>> &p_default_textures, const Vector<ShaderCompiler::GeneratedCode::Texture> &p_texture_uniforms, RID *p_textures, bool p_use_linear_color, bool p_3d_material) {
    ...
    for (int i = 0, k = 0; i < p_texture_uniforms.size(); i++) {
        const StringName &uniform_name = p_texture_uniforms[i].name;
        ...
        HashMap<StringName, Variant>::ConstIterator V = p_parameters.find(uniform_name);
        if (V) {
            ...
        }
        ...
        for (int j = 0; j < textures.size(); j++) {
            TextureStorage::Texture *tex = TextureStorage::get_singleton()->get_texture(textures[j]);
            ...
            if (rd_texture.is_null()) {
                rd_default = get_default_texture_id(p_texture_uniforms[i].type, p_texture_uniforms[i].hint);
            }
        }
    }
}
```

**Verdict:** Godot binds textures by *name* (`uniform_name`), not by *slot* / descriptor; there is no neutral descriptor — the `RID` and texture storage are renderer-side (`TextureStorage`).

---

## 6. Device/context loss & reload

- `Shader::set_code` (in `scene/resources/shader.cpp`) re-preprocesses the code, pushes to `shader_set_code` if the RID is valid, and calls `emit_changed()` (symbol: `Shader::set_code`):
  ```cpp
  void Shader::set_code(const String &p_code) {
      ...
      if (shader_rid.is_valid()) {
          RenderingServer::get_singleton()->shader_set_code(shader_rid, preprocessed_code);
          preprocessed_code = String();
      }
      emit_changed();
  }
  ```
- `Shader::set_default_texture_parameter` also calls `emit_changed()` (symbol: `Shader::set_default_texture_parameter`):
  ```cpp
  void Shader::set_default_texture_parameter(const StringName &p_name, const Ref<Texture> &p_texture, int p_index) {
      ...
      RS::get_singleton()->shader_set_default_texture_parameter(shader_rid, p_name, p_texture->get_rid(), p_index);
      ...
      emit_changed();
  }
  ```
- `Shader::_dependency_changed` / `Shader::_recompile` are the recompile paths for `ShaderInclude` dependencies:
  ```cpp
  void Shader::_dependency_changed() {
      // Preprocess and compile the code again because a dependency has changed. It also calls emit_changed() for us.
      _recompile();
  }
  void Shader::_recompile() {
      set_code(get_code());
  }
  ```
- `MaterialStorage::material_set_shader` (server-side, line 2515) frees old shader data and creates a new `MaterialData` based on shader type:
  ```cpp
  material->data = material_data_request_func[shader->type](shader->data);
  ```
- `MaterialStorage::_material_uniform_set_erased` (line 2432) triggers a rebuild of the uniform set when a texture is deleted:
  ```cpp
  void MaterialStorage::_material_uniform_set_erased(void *p_material) {
      RID rid = *(RID *)p_material;
      Material *material = MaterialStorage::get_singleton()->get_material(rid);
      if (material) {
          if (material->data) {
              // Uniform set may be gone because a dependency was erased. This happens
              // if a texture is deleted, so re-create it.
              MaterialStorage::get_singleton()->_material_queue_update(material, false, true);
          }
          material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
      }
  }
  ```
- `Material::_validate_property` (`scene/resources/material.cpp`) only alters property metadata in the editor (hiding `next_pass` / `render_priority`), it does not rebuild GPU state.

**Verdict:** Godot has no centralized `ResourceContext` / device-loss reload; invalidation is driven by the server side (dependency tracker, `_material_uniform_set_erased`) and resource-side (`emit_changed()`).

---

## 7. Editor thumbnail handling

The prompt's suggested file (`editor/plugins/editor_resource_preview.{h,cpp}`) does not exist in the current master branch layout. Based on the GitHub directory listing of `editor/plugins`, the contents are:
- `editor_plugin.cpp/h`
- `editor_plugin_list.cpp/h`
- `editor_plugin_settings.cpp/h`
- `editor_resource_conversion_plugin.cpp/h`
- `plugin_config_dialog.cpp/h`

**Note:** The specific source of `EditorResourcePreview` was not located in the paths probed (it may have moved or been split). I cannot confidently cite the exact source for the thumbnail cache generation/caching from primary sources. I would need a follow-up search of the `editor/` tree (e.g. `editor/` root or `editor/` specific subdirectories).

---

## Differences worth stealing / lessons

1. **Lazy RID materialization (`_check_material_rid()`)** — Our model's `prepare()` should match Godot's `ShaderMaterial::get_rid()`/`_check_material_rid()` pattern: hold an `RID` that stays null until first `get_rid()`, and defer to a `ResourceContext` (like Godot's `MaterialStorage`) on creation. The "deferred + params" staging (`pending_params`) is the right approach for `BaseMaterial3D`.
2. **Server-side staging is a HashMap + dirty flags, not a fixed-size staging buffer.** Godot writes params into `material->params` and queues an update with `uniform_dirty`/`texture_dirty`. This is closer to a "pull-based staging" than a fixed-size 256-byte-aligned staging buffer. If we want a 256-byte-aligned staging buffer, we are doing something Godot does not (Godot uses `std140`-computed UBO sizes).
3. **Name-based texture resolution (`MaterialData::update_textures`)** — Godot binds textures by `uniform_name` from `p_parameters`. This is the same as our name-based texture slots, but Godot has *no neutral descriptor*; it takes raw `RID` and `TextureStorage`. Our model should keep the neutral descriptor (`IResource` + `ResourceContext`), but Godot's approach is a good baseline for the "name → RID" resolution path.
4. **`emit_changed()` is a resource-side event, but GPU-side invalidation is driven by `DependencyTracker`.** Godot's `_material_uniform_set_erased` (server side) triggers a rebuild when a texture is deleted. Our model should have a `ResourceContext` (renderer-side) to track dependencies and queue rebuilds instead of relying only on `Resource::emit_changed()`.
5. **Server-side `material_free` explicitly clears texture arrays to prevent spin locking of RIDs.** If we have a ResourceContext with a similar texture list, we should clear texture RIDs on `release()` to prevent dangling RIDs.
6. **Server-side invalidation is queued, not immediate.** Godot's `_material_queue_update` / `_update_queued_materials` drains the queue at a frame boundary. Our `prepare()` should be idempotent and queue-based, not immediate.
7. **`Shader::set_code` triggers server-side `shader_set_code` + `emit_changed()`** — the "hot reload" path is: resource-side `set_code` → preprocess → `shader_set_code` (server-side recompile) → `emit_changed()`. Our model's `get_revision()` should bump on shader reload and trigger server-side rebuild via a dirty queue.
8. **Editor placeholder (`CompressedTexture2D::get_rid` → `texture_2d_placeholder_create`)** — Godot uses a placeholder texture when the resource isn't ready to avoid a hard crash. Our model should have a "default" texture/invalid RID strategy for placeholder textures.
9. **Server-side `MaterialStorage` has no concept of "neutral descriptor"** — it takes `RID`s and raw `Variant`s. If we want renderer-neutral descriptors, we have to build the descriptor layer ourselves; Godot's approach is *tighter to the renderer* but less portable.
10. **`MaterialStorage::material_set_param` disallows `Variant::OBJECT`** (`ERR_FAIL_COND(p_value.get_type() == Variant::OBJECT)`) — our model should either support object params or explicitly reject them with a neutral wrapper, but Godot's strictness (not allowing objects as params) is a good guard for the "name → RID" texture path.
