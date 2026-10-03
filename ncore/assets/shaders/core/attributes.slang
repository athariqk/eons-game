// Material render-state attributes.
// Applied to structs implementing ISpatialMaterial or ICanvasMaterial.
// Read at compile time by ShaderCompiler reflection → MaterialShaderDesc.

[__AttributeUsage(_AttributeTargets.Struct)]
struct CullModeAttribute
{
    int mode; // 0=NONE, 1=FRONT, 2=BACK
};

[__AttributeUsage(_AttributeTargets.Struct)]
struct FillModeAttribute
{
    int mode; // 0=SOLID, 1=WIREFRAME
};

[__AttributeUsage(_AttributeTargets.Struct)]
struct DepthTestAttribute
{
    int enabled;
};

[__AttributeUsage(_AttributeTargets.Struct)]
struct DepthWriteAttribute
{
    int enabled;
};

[__AttributeUsage(_AttributeTargets.Struct)]
struct BlendAttribute
{
    int preset; // 0=OPAQUE, 1=ALPHA_BLEND, 2=ALPHA_PREMULTIPLIED, 3=ADDITIVE
};

