# Esoterica-style materials in ncore

## Model (from Esoterica Engine)

```text
MaterialShader  = Shader bytecode + MaterialShaderFlags + reflected param layout
Material        = shader id + parameter values (textures / scalars)
Pass / buckets  = Opaque | AlphaTest | AlphaBlend  (+ depth-only permutation later)
```

Philosophy: **shaders are code**; materials do not author full PSOs; few unique shaders.

## ncore mapping

| Esoterica | ncore |
|-----------|--------|
| `MaterialShader` | `Shader` + `MaterialShaderFlags` / `MaterialShaderDesc` |
| `MaterialShaderFlags` | `AlphaTest`, `AlphaBlend`, `TwoSided` |
| `Material` resource | `MaterialTemplate` (thin) |
| Parameter storage | `MaterialParameterStorage` + `MaterialComponent::Params` |
| Shader buckets | `MaterialDrawBucket` (sort key for draw lists) |
| Depth-only permutation | TODO: specialized compile / second PS |

## Authoring

**Shader** (`// nc_pipeline:` still works):

```slang
// nc_pipeline: cull=back, blend=alpha, depth_test=true, depth_write=true
```

**Material** `.material` (Esoterica-thin):

```ini
[header]
debug_name = Water

[shader]
path = shaders/water.slang

[flags]
surface = AlphaBlend

[vertex]
layout = Vertex3D
```

`[raster]` is ignored.

## Runtime

1. Load `MaterialTemplate` → resolve flags from `[flags]` or shader policy.
2. `material_create(tmpl)` → PSO via `surface_policy_from_flags` + `PassPipelineDefaults`.
3. Instance textures / `Params` on `MaterialComponent`.
4. Draw list sorts by `draw_bucket()` then shader.

## Local fix required

`render_storage.cpp` on this branch may be truncated. Restore from master and replace `get_pso_key_` so it uses:

```cpp
SurfacePolicy surface = tmpl.shader->get_surface_policy();
if (tmpl.flags != MaterialShaderFlags::None)
    surface = surface_policy_from_flags( tmpl.flags, surface );
ResolvedSurfaceState resolved = resolve_surface( surface, {}, PassPipelineDefaults{} );
key.flags = static_cast<PSOFlags>( encode_pso_flags( resolved, pass ) );
```

Include `material_shader.h` and `surface_policy.h`.

## Follow-ups

- ShaderCompiler: parse `// nc_pipeline:` → `ShaderDesc::surface_policy`.
- GPU parameter buffer pool (Esoterica 32-byte block offsets).
- Depth-only permutation for shadows.
- Scene plugin: push `ParamsDirty` via `material_set_params`.
