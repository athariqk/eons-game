# Materials refactor prototype (`grok/materials-refactor`)

## Problem

`MaterialTemplate` currently owns both:

- shader reference
- full fixed-function PSO fields (cull, depth, blend, MSAA, …)

That couples **surface intent** to **pass-specific pipeline state** and fights the day one shader is drawn in depth-prepass, forward, and shadow with different depth/blend.

## Target split (industry-typical)

| Layer | Owns | Prototype types |
|-------|------|-----------------|
| **Shader** | Code, resources, optional *surface defaults* | `ShaderDesc::surface_policy`, `// nc_pipeline:` |
| **Material instance** | Textures, param blob, rare overrides | `MaterialSurfaceOverrides`, existing `MaterialComponent` |
| **Pass / draw list** | Depth, RT formats, forced blend for special passes | `PassPipelineDefaults` |
| **PSO cache** | Combined key | `encode_pso_flags(surface, pass)` → existing `PSOFlags` |

Slang **specialization** stays for *code* variants (features), not cull/blend.

## Authoring hint

In `.slang` (usually near the fragment entry):

```slang
// nc_pipeline: cull=back, fill=solid, blend=opaque, alpha=opaque, double_sided=false
```

Parser: `ncore/include/ncore/services/io/pipeline_hint_parser.h`  
`parse_pipeline_hint_from_source(source, policy)` — last matching line wins.

Wire-up (next step, not fully hooked on this branch):

1. `ShaderCompiler` reads file text → parse hint → set `ShaderDesc::surface_policy`.
2. `material_create` takes `Shader` + optional overrides instead of full `MaterialTemplate` PSO list.
3. Spatial/canvas passes supply `PassPipelineDefaults` when building `PSOKey`.

## Bridge from legacy `MaterialTemplate`

```cpp
auto surface = resolve_surface(shader->get_surface_policy(), overrides);
PassPipelineDefaults pass{ /* from camera/pass */ };
auto flags = encode_pso_flags(surface, pass);
// or apply_to_legacy_template_fields(...) then existing get_pso_key_(tmpl)
```

## What this branch does *not* do yet

- Remove `MaterialTemplate` or break existing scenes
- Parse real Slang `[pipeline(...)]` attributes (comment form only)
- Change `material_create` signature
- Pass-level PSO construction in `scene_plugins`

## Example

See `assets/shaders/prototype/surface_policy_example.slang`.
