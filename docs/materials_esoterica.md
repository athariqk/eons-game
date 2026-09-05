# Materials without MaterialTemplate

## Model

```text
Shader resource          // .slang  (+ optional // nc_pipeline:)
MaterialCreateDesc       // flags, vertex layout name, debug name
MaterialComponent        // Source = Shader RID, textures, Params, Flags
GPU material (RID)       // material_create(shader, desc)
```

There is **no** `MaterialTemplate` type. `.material` files are obsolete; load shaders directly.

## API

```cpp
RID material_create( const Shader& shader, const MaterialCreateDesc& desc = {} );
```

```cpp
MaterialComponent mat;
mat.Source = resources->load( "shaders/water.slang" );
mat.Flags  = MaterialShaderFlags::AlphaBlend;
mat.VertexLayoutName = "Vertex3D";
mat.add_texture( gpu_tex );
```

## Flags (Esoterica)

- `TwoSided`
- `AlphaTest`
- `AlphaBlend`

Buckets: `draw_bucket_from_flags` → Opaque / AlphaTest / AlphaBlend.

## Migration

| Old | New |
|-----|-----|
| `load("materials/x.material")` | `load("shaders/x.slang")` |
| `get<MaterialTemplate>` | `get<Shader>` |
| `material_create(*tmpl)` | `material_create(*shader, desc)` |
| `[raster]` in ini | `// nc_pipeline:` or `Flags` |
