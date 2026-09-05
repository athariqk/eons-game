# Materials total replacement (`grok/materials-refactor`)

## What changed

`MaterialTemplate` **no longer stores** cull / fill / depth / blend / MSAA.

| Before | After |
|--------|--------|
| `[raster]` in `.material` | `// nc_pipeline:` in `.slang` |
| PSO fields on template | `SurfacePolicy` on `Shader` + `PassPipelineDefaults` |
| `get_pso_key_(tmpl)` reads tmpl.cull_mode… | `resolve_surface` + `encode_pso_flags` |

`MaterialTemplate` is now:

```text
debug_name
vertex_layout_name   (optional; else VS reflection)
shader               (Ref<Shader>)
```

## Authoring

```slang
// nc_pipeline: cull=back, fill=solid, blend=opaque, depth_test=true, depth_write=true
```

Keys: `cull`, `fill`, `blend`, `alpha`, `double_sided`, `depth_test`, `depth_write`.

`.material` files only need `[header]`, `[shader] path=…`, optional `[vertex] layout=…`.
`[raster]` / `[multisample]` are ignored (warning).

## Runtime

```text
Shader.surface_policy     // from parser at compile
  + MaterialSurfaceOverrides  // e.g. wireframe via material_set_draw_mode
  + PassPipelineDefaults      // RT formats, force depth/blend for special passes
  → PSOFlags / PSOKey
```

Stock shaders updated: `canvas.slang`, `world_object.slang`.  
Seaengine: add the same line to `skybox.slang` / `water.slang` (and strip raster from their `.material` files when convenient).

## Follow-ups

1. Hook `parse_pipeline_hint_from_source` in `ShaderCompiler::compile` (read file text → fill every `ShaderDesc::surface_policy`).
2. `get_pso_key_` / `material_create` use `encode_pso_flags` (see render_storage changes on this branch when present).
3. Optional: load `Shader` directly as `MaterialComponent::Source` and drop `.material` entirely.
