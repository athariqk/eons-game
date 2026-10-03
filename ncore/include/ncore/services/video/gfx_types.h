#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/utils/enum.h>

#pragma push_macro( "NONE" )
#pragma push_macro( "OPAQUE" )
#undef NONE
#undef OPAQUE

namespace nc::gfx {

enum class PrimitiveTopology {
    TRIANGLE_LIST,
    POINT_LIST,
};

enum class FillMode {
    SOLID,
    WIREFRAME
};
NENUM( FillMode, NENUM_ELEMENT( FillMode, SOLID ), NENUM_ELEMENT( FillMode, WIREFRAME ) )

/**
 * @brief This enumeration describes which parts of the pipeline a resource can be bound to.
 */
enum class ResourceBindFlags : uint32_t {
    NONE               = 0,
    VERTEX_BUFFER      = 1 << 0,
    INDEX_BUFFER       = 1 << 1,
    UNIFORM_BUFFER     = 1 << 2, // A constant buffer. may not be combined with any other bind flag (Diligent Impl)
    SHADER_RESOURCE    = 1 << 3, // A buffer or a texture can be bound as a shader resource.
    STREAM_OUTPUT      = 1 << 4,
    RENDER_TARGET      = 1 << 5,
    DEPTH_STENCIL      = 1 << 6,
    UNORDERED_ACCESS   = 1 << 7, // A buffer or a texture can be bound as an unordered access view.
    INDIRECT_DRAW_ARGS = 1 << 8,
    INPUT_ATTACHMENT   = 1 << 9,
    RAY_TRACING        = 1 << 10,
    SHADING_RATE
};
ENABLE_BITMASK( ResourceBindFlags )
NENUM(
    ResourceBindFlags, NENUM_ELEMENT( ResourceBindFlags, NONE ), NENUM_ELEMENT( ResourceBindFlags, VERTEX_BUFFER ),
    NENUM_ELEMENT( ResourceBindFlags, INDEX_BUFFER ), NENUM_ELEMENT( ResourceBindFlags, UNIFORM_BUFFER ),
    NENUM_ELEMENT( ResourceBindFlags, SHADER_RESOURCE ), NENUM_ELEMENT( ResourceBindFlags, STREAM_OUTPUT ),
    NENUM_ELEMENT( ResourceBindFlags, RENDER_TARGET ), NENUM_ELEMENT( ResourceBindFlags, DEPTH_STENCIL ),
    NENUM_ELEMENT( ResourceBindFlags, UNORDERED_ACCESS ), NENUM_ELEMENT( ResourceBindFlags, INDIRECT_DRAW_ARGS ),
    NENUM_ELEMENT( ResourceBindFlags, INPUT_ATTACHMENT ), NENUM_ELEMENT( ResourceBindFlags, RAY_TRACING ),
    NENUM_ELEMENT( ResourceBindFlags, SHADING_RATE )
)

enum class ResourceUsage : uint8_t {
    /**
     * @brief A resource that can only be read by the GPU. It cannot be written by the GPU,
     * and cannot be accessed at all by the CPU.
     *
     * This type of resource must be initialized when it is created, since it cannot be changed after creation.
     */
    IMMUTABLE = 0,
    /**
     * @brief Can be read and written into by CPU.
     */
    DEFAULT,
    /**
     * @brief Can be read and written into by CPU at least once per frame.
     */
    DYNAMIC,
    STAGING,
    /**
     * @brief A resource residing in a unified memory (e.g. memory shared between CPU and GPU).
     *
     * Can be read and written by GPU and can also be directly accessed by CPU.
     */
    UNIFIED,
    /**
     * @brief A resource that can be partially committed to physical memory.
     */
    SPARSE
};
NENUM(
    ResourceUsage, NENUM_ELEMENT( ResourceUsage, IMMUTABLE ), NENUM_ELEMENT( ResourceUsage, DEFAULT ),
    NENUM_ELEMENT( ResourceUsage, DYNAMIC ), NENUM_ELEMENT( ResourceUsage, STAGING ),
    NENUM_ELEMENT( ResourceUsage, UNIFIED ), NENUM_ELEMENT( ResourceUsage, SPARSE )
)

enum class ColorMask : uint8_t {
    NONE  = 0,
    RED   = 1 << 0,
    GREEN = 1 << 1,
    BLUE  = 1 << 2,
    ALPHA = 1 << 3,
    RGB   = RED | GREEN | BLUE,
    ALL   = RGB | ALPHA
};
ENABLE_BITMASK( ColorMask )

/**
 * @brief Allowed CPU access mode flags when mapping a resource.
 */
enum class ResourceCpuAccessFlags : uint8_t {
    NONE       = 0,           // No CPU access
    READ       = 1 << 0,      // Resource can be mapped for reading.
    WRITE      = 1 << 1,      // Resource can be mapped for writing.
    READ_WRITE = READ | WRITE // Resource can be mapped for reading and writing.
};
ENABLE_BITMASK( ResourceCpuAccessFlags )
NENUM(
    ResourceCpuAccessFlags, NENUM_ELEMENT( ResourceCpuAccessFlags, NONE ),
    NENUM_ELEMENT( ResourceCpuAccessFlags, READ ), NENUM_ELEMENT( ResourceCpuAccessFlags, WRITE ),
    NENUM_ELEMENT( ResourceCpuAccessFlags, READ_WRITE )
)

enum class TextureFormat {
    RGBA8_UNORM,
    RGBA8_UNORM_SRGB,
    R32_FLOAT,
    RG32_FLOAT,
    RGBA32_FLOAT,
    D32_FLOAT, // Depth format single-float.
    UNKNOWN,   // Can also mean no texture.
};
NENUM(
    TextureFormat, NENUM_ELEMENT( TextureFormat, RGBA8_UNORM ), NENUM_ELEMENT( TextureFormat, RGBA8_UNORM_SRGB ),
    NENUM_ELEMENT( TextureFormat, R32_FLOAT ), NENUM_ELEMENT( TextureFormat, RG32_FLOAT ),
    NENUM_ELEMENT( TextureFormat, RGBA32_FLOAT ), NENUM_ELEMENT( TextureFormat, D32_FLOAT ),
    NENUM_ELEMENT( TextureFormat, UNKNOWN )
)

enum class ResourceDimension {
    DIM_1D,
    DIM_1D_ARRAY,
    DIM_2D,
    DIM_2D_ARRAY,
    DIM_3D,
    DIM_CUBE,
    DIM_CUBE_ARRAY
};

enum class VertexFrequency {
    PER_VERTEX,
    PER_INSTANCE,
};

enum class CullMode {
    NONE,
    FRONT,
    BACK,
};

enum class BlendPreset {
    OPAQUE,
    ALPHA_BLEND,
    ALPHA_PREMULTIPLIED,
    ADDITIVE,
};

enum class BlendFactor {
    ZERO,
    ONE,
    SRC_COLOR,
    INV_SRC_COLOR,
    SRC_ALPHA,
    INV_SRC_ALPHA,
    DST_COLOR,
    INV_DST_COLOR,
    DST_ALPHA,
    INV_DST_ALPHA,
    CONSTANT_COLOR,
};

enum class BlendOp {
    ADD,
    SUBTRACT,
    REV_SUBTRACT,
    MIN,
    MAX
};

enum class CompareFunc {
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS
};

enum class StencilOp {
    KEEP,
    ZERO,
    REPLACE,
    INCR_CLAMP,
    DECR_CLAMP,
    INVERT,
    INCR_WRAP,
    DECR_WRAP
};

enum class SamplerFilter {
    NEAREST,
    LINEAR,
    ANISOTROPIC,
};

enum class TextureAddressMode {
    WRAP,
    CLAMP,
    MIRROR,
    BORDER,
};

/**
 * @brief The type of a shader resource.
 */
enum class ResourceType {
    UNKNOWN,         // Shader resource type is unknown.
    CONSTANT_BUFFER, // Constant (uniform) buffer.
    TEXTURE_SRV,     // Shader resource view of a texture (sampled image).
    BUFFER_SRV,      // Shader resource view of a buffer (read-only storage image).
    TEXTURE_UAV,     // Unordered access view of a texture (storage image).
    BUFFER_UAV,      // Unordered access view of a buffer (storage buffer).
    SAMPLER,
    VARYING_INPUT,
};
NENUM(
    ResourceType, NENUM_ELEMENT( ResourceType, UNKNOWN ), NENUM_ELEMENT( ResourceType, CONSTANT_BUFFER ),
    NENUM_ELEMENT( ResourceType, TEXTURE_SRV ), NENUM_ELEMENT( ResourceType, BUFFER_SRV ),
    NENUM_ELEMENT( ResourceType, TEXTURE_UAV ), NENUM_ELEMENT( ResourceType, BUFFER_UAV ),
    NENUM_ELEMENT( ResourceType, SAMPLER ), NENUM_ELEMENT( ResourceType, VARYING_INPUT ),
)

/**
 * @brief The binding type of a shader resource.
 */
enum class ResourceBindType {
    STATIC,
    MUTABLE,
    DYNAMIC,
};

enum class ResourceFlags : uint8_t {
    NONE               = 0,
    NO_DYNAMIC_BUFFERS = 1 << 0,
    COMBINED_SAMPLER   = 1 << 1,
    FORMATTED_BUFFER   = 1 << 2,
    /**
     * @brief Indicates that the resource consists of inline constants (also
     * known as push constants in Vulkan or root constants in Direct3D12).
     *
     * Applies to CONSTANT_BUFFER resource type only.
     */
    INLINE_CONSTANTS = 1 << 3,
    /**
     * @brief Indicates that the resource is a run-time sized shader array.
     *
     * Requires the device to support ShaderResourceRuntimeArrays feature
     * (always enabled on D3D12, requires runtimeDescriptorArray on Vulkan).
     */
    RUNTIME_ARRAY = 1 << 4
};
ENABLE_BITMASK( ResourceFlags )
NENUM(
    ResourceFlags, NENUM_ELEMENT( ResourceFlags, NONE ), NENUM_ELEMENT( ResourceFlags, NO_DYNAMIC_BUFFERS ),
    NENUM_ELEMENT( ResourceFlags, COMBINED_SAMPLER ), NENUM_ELEMENT( ResourceFlags, FORMATTED_BUFFER ),
    NENUM_ELEMENT( ResourceFlags, INLINE_CONSTANTS ), NENUM_ELEMENT( ResourceFlags, RUNTIME_ARRAY )
)

/**
 * @brief This relates to how buffers are interpreted for read operations.
 */
enum class BufferMode : uint8_t {
    NONE = 0,
    FORMATTED,
    STRUCTURED,
    RAW
};

enum class SwapChainUsage : uint32_t {
    NONE             = 0,
    RENDER_TARGET    = 1 << 0,
    SHADER_RESOURCE  = 1 << 1,
    INPUT_ATTACHMENT = 1 << 2,
    COPY_SOURCE      = 1 << 3,
};
ENABLE_BITMASK( SwapChainUsage )

enum class TextureViewType {
    SHADER_RESOURCE,
    RENDER_TARGET,
    DEPTH_STENCIL,
    UNORDERED_ACCESS,
};

enum class BufferViewType {
    SHADER_RESOURCE, // for shader read operations.
    UNORDERED_ACCESS // for unordered read/write operations from shaders.
};
NENUM(
    BufferViewType, NENUM_ELEMENT( BufferViewType, SHADER_RESOURCE ), NENUM_ELEMENT( BufferViewType, UNORDERED_ACCESS )
)

/**
 * @brief The part of the pipeline a shader will live.
 */
enum class ShaderStage : uint8_t {
    NONE    = 0,
    VERTEX  = 1 << 0,
    PIXEL   = 1 << 1,
    COMPUTE = 1 << 2,
    VS_PS   = VERTEX | PIXEL
};
ENABLE_BITMASK( ShaderStage )
NENUM(
    ShaderStage, NENUM_ELEMENT( ShaderStage, NONE ), NENUM_ELEMENT( ShaderStage, VERTEX ),
    NENUM_ELEMENT( ShaderStage, PIXEL ), NENUM_ELEMENT( ShaderStage, COMPUTE ), NENUM_ELEMENT( ShaderStage, VS_PS )
)

enum class ParameterType : uint32_t {
    NONE = 0,

    // Base Types (Bits 0-7)
    FLOAT_16 = 1 << 0,
    FLOAT_32 = 1 << 1,
    FLOAT_64 = 1 << 2,
    INT_8    = 1 << 3,
    INT_16   = 1 << 4,
    INT_32   = 1 << 5,
    INT_64   = 1 << 6,
    BOOL     = 1 << 7,

    // Vector / Matrix Component Counts (Bits 8-10)
    VEC2 = 1 << 8,
    VEC3 = 1 << 9,
    VEC4 = 1 << 10,

    // Qualifiers & Formats (Bits 11-12)
    UNSIGNED   = 1 << 11,
    NORMALIZED = 1 << 12,

    // Aliases
    FLOAT = FLOAT_32, // 32-bit signed float
    LONG  = INT_64,   // 64-bit signed integer
    INT   = INT_32,   // 32-bit signed integer
    SHORT = INT_16,   // 16-bit signed integer
    BYTE  = INT_8,    // 8-bit signed integer

    // Scalar Compositions
    UFLOAT = FLOAT | UNSIGNED, // 32-bit unsigned float
    ULONG  = LONG | UNSIGNED,  // 64-bit unsigned integer
    UINT   = INT | UNSIGNED,   // 32-bit unsigned integer
    USHORT = SHORT | UNSIGNED, // 16-bit unsigned integer
    UBYTE  = BYTE | UNSIGNED,  // 8-bit unsigned integer

    // Vector Compositions
    FLOAT2  = FLOAT | VEC2,
    FLOAT3  = FLOAT | VEC3,
    FLOAT4  = FLOAT | VEC4,
    INT2    = INT | VEC2,
    INT3    = INT | VEC3,
    INT4    = INT | VEC4,
    USHORT4 = USHORT | VEC4,

    // Special Packed Formats
    BYTE4_NORM  = BYTE | VEC4 | NORMALIZED,
    UBYTE4_NORM = BYTE4_NORM | UNSIGNED,

    // Matrices (Bits 13-14)
    MAT3 = 1 << 13,
    MAT4 = 1 << 14,

    // Textures & Samplers (Bits 15-19)
    TEXTURE_2D          = 1 << 15,
    TEXTURE_2D_ARRAY    = 1 << 16,
    TEXTURE_CUBED       = 1 << 17,
    TEXTURE_CUBED_ARRAY = 1 << 18,
    SAMPLER             = 1 << 19,
    TEXTURES            = TEXTURE_2D | TEXTURE_2D_ARRAY | TEXTURE_CUBED | TEXTURE_CUBED_ARRAY,

    UNKNOWN = 0xFFFFFFFF
};
ENABLE_BITMASK( ParameterType )
NENUM(
    ParameterType, NENUM_ELEMENT( ParameterType, NONE ), NENUM_ELEMENT( ParameterType, FLOAT_16 ),
    NENUM_ELEMENT( ParameterType, FLOAT_32 ), NENUM_ELEMENT( ParameterType, FLOAT_64 ),
    NENUM_ELEMENT( ParameterType, INT_8 ), NENUM_ELEMENT( ParameterType, INT_16 ),
    NENUM_ELEMENT( ParameterType, INT_32 ), NENUM_ELEMENT( ParameterType, INT_64 ),
    NENUM_ELEMENT( ParameterType, BOOL ), NENUM_ELEMENT( ParameterType, VEC2 ), NENUM_ELEMENT( ParameterType, VEC3 ),
    NENUM_ELEMENT( ParameterType, VEC4 ), NENUM_ELEMENT( ParameterType, MAT3 ), NENUM_ELEMENT( ParameterType, MAT4 ),
    NENUM_ELEMENT( ParameterType, UNSIGNED ), NENUM_ELEMENT( ParameterType, NORMALIZED ),
    NENUM_ELEMENT( ParameterType, FLOAT ), NENUM_ELEMENT( ParameterType, INT ), NENUM_ELEMENT( ParameterType, SHORT ),
    NENUM_ELEMENT( ParameterType, BYTE ), NENUM_ELEMENT( ParameterType, UFLOAT ), NENUM_ELEMENT( ParameterType, UINT ),
    NENUM_ELEMENT( ParameterType, USHORT ), NENUM_ELEMENT( ParameterType, UBYTE ),
    NENUM_ELEMENT( ParameterType, FLOAT2 ), NENUM_ELEMENT( ParameterType, FLOAT3 ),
    NENUM_ELEMENT( ParameterType, FLOAT4 ), NENUM_ELEMENT( ParameterType, INT2 ), NENUM_ELEMENT( ParameterType, INT3 ),
    NENUM_ELEMENT( ParameterType, INT4 ), NENUM_ELEMENT( ParameterType, USHORT4 ),
    NENUM_ELEMENT( ParameterType, BYTE4_NORM ), NENUM_ELEMENT( ParameterType, UBYTE4_NORM ),
    NENUM_ELEMENT( ParameterType, TEXTURE_2D ), NENUM_ELEMENT( ParameterType, TEXTURE_2D_ARRAY ),
    NENUM_ELEMENT( ParameterType, TEXTURE_CUBED ), NENUM_ELEMENT( ParameterType, TEXTURE_CUBED_ARRAY ),
    NENUM_ELEMENT( ParameterType, SAMPLER ), NENUM_ELEMENT( ParameterType, TEXTURES ),
    NENUM_ELEMENT( ParameterType, UNKNOWN )
)

struct SwapChainDesc {
    void* native_whnd = nullptr;
    Vec2i initial_size;
    SwapChainUsage usage       = SwapChainUsage::RENDER_TARGET;
    bool is_primary            = false;
    int buffer_count           = 2;
    TextureFormat color_format = TextureFormat::RGBA8_UNORM_SRGB;
    TextureFormat depth_format = TextureFormat::D32_FLOAT;
};

/**
 * @brief Describes a single bindable resource (e.g. one constant buffer).
 *
 * Analogous to one entry in an HLSL/Slang root/descriptor table slot.
 */
struct PipelineResourceDesc {
    /**
     * @brief Shader-side variable name.
     */
    String name;
    /**
     * @brief Which shader stage see this resource.
     */
    ShaderStage stage          = ShaderStage::NONE;
    ResourceType resource_type = ResourceType::CONSTANT_BUFFER;
    /**
     * @brief Controls how often the binding can change and how it's optimized internally.
     */
    ResourceBindType var_type = ResourceBindType::DYNAMIC;
    /**
     * @brief Extra bits.
     */
    ResourceFlags flags = ResourceFlags::NONE;
    /**
     * @brief Resource array size (must be 1 for non-array resources).
     *
     * e.g. Texture2D g_Textures[4]
     */
    uint32_t array_size = 1;
};

typedef uint16_t BindingSet;

/**
 * @brief Describes a descriptor set or D3D12 root signature table.
 */
struct ResourceSignatureDesc {
    String name;
    BindingSet set_idx = 0; // Each resource signature should have different set index.
    DynamicArray<PipelineResourceDesc> resources;
};

/**
 * @brief All the descriptor sets of a shader.
 */
using ResourceLayoutDesc = HashMap<BindingSet, ResourceSignatureDesc>;

struct ResourceMappingEntry {
    const char* variable_name; // Must match the shader param/field name exactly.
    ResourceType kind;
    RID resource;              // Resource to bind.
    uint32_t array_index = 0;  // For array resources, index in the array.
};

struct BufferDesc {
    String debug_name;
    ResourceBindFlags bind_mask;
    size_t size;
    const void* initial_data               = nullptr;
    ResourceUsage usage                    = ResourceUsage::DEFAULT;
    ResourceCpuAccessFlags cpu_access_mask = ResourceCpuAccessFlags::NONE;
    BufferMode mode                        = BufferMode::RAW;
    /**
     * @brief Byte size of one element. Required when mode is STRUCTURED or FORMATTED.
     */
    size_t element_stride = 0;
};

struct TextureDataDesc {
    const void* pixels = nullptr;
};

struct TextureDesc {
    String debug_name;
    TextureFormat format                   = TextureFormat::RGBA8_UNORM_SRGB;
    ResourceDimension dimension            = ResourceDimension::DIM_2D;
    ResourceUsage usage                    = ResourceUsage::DEFAULT;
    ResourceBindFlags bind_mask            = ResourceBindFlags::SHADER_RESOURCE;
    ResourceCpuAccessFlags cpu_access_mask = ResourceCpuAccessFlags::NONE;
    uint32_t width                         = 0;
    uint32_t height                        = 0;
    uint32_t array_size                    = 1;
    uint32_t mip_levels                    = 1;
    uint32_t sample_count                  = 1;
    DynamicArray<TextureDataDesc> subresources;
};

struct VertexLayoutElement {
    uint32_t location;
    ParameterType type;
    uint32_t buffer_slot     = 0;
    size_t stride            = 0;
    uint32_t relative_offset = ~0u;
    bool normalized          = false;
    VertexFrequency frequency;
    uint32_t instance_step_rate = 1;
    StringView hlsl_semantic    = "ATTRIB";
};

using VertexLayout = DynamicArray<VertexLayoutElement>;

struct StencilOpDesc {
    StencilOp fail       = StencilOp::KEEP;
    StencilOp depth_fail = StencilOp::KEEP;
    StencilOp pass       = StencilOp::KEEP;
    CompareFunc func     = CompareFunc::ALWAYS;
};

struct DepthStencilStateDesc {
    bool depth_test            = true;
    bool depth_write           = true;
    CompareFunc depth_func     = CompareFunc::LESS_EQUAL;
    bool stencil_test          = false;
    uint8_t stencil_read_mask  = 0xFF;
    uint8_t stencil_write_mask = 0xFF;
    StencilOpDesc front, back;
};

struct RasterizerStateDesc {
    CullMode cull             = CullMode::BACK;
    FillMode fill             = FillMode::SOLID;
    bool front_ccw            = true;
    float depth_bias_constant = 0.0f;
    float depth_bias_slope    = 0.0f;
    float depth_bias_clamp    = 0.0f;
    bool depth_clamp_enable   = false;
    bool scissor_enable       = false;
};

struct RenderTargetBlendDesc {
    bool enable           = false;
    BlendFactor src_color = BlendFactor::ONE;
    BlendFactor dst_color = BlendFactor::ZERO;
    BlendOp op_color      = BlendOp::ADD;
    BlendFactor src_alpha = BlendFactor::ONE;
    BlendFactor dst_alpha = BlendFactor::ZERO;
    BlendOp op_alpha      = BlendOp::ADD;
    ColorMask write_mask  = ColorMask::ALL;
};

struct BlendStateDesc {
    Array<RenderTargetBlendDesc, 8> render_targets;
    bool alpha_to_coverage = false;
};

struct MultisampleStateDesc {
    uint8_t count;
    uint8_t quality;
};

struct SamplerDesc {
    String debug_name;
    SamplerFilter mag_filter     = SamplerFilter::LINEAR;
    SamplerFilter min_filter     = SamplerFilter::LINEAR;
    SamplerFilter mip_filter     = SamplerFilter::LINEAR;
    TextureAddressMode address_u = TextureAddressMode::WRAP;
    TextureAddressMode address_v = TextureAddressMode::WRAP;
    TextureAddressMode address_w = TextureAddressMode::WRAP;
};

/**
 * @brief Shader creation description.
 */
struct ShaderCreateDesc {
    StringView name;
    gfx::ShaderStage stage;
    Span<const uint32_t> bytecode;
};

struct RenderPipelineDesc {
    String debug_name;
    TextureFormat render_target_format   = TextureFormat::RGBA8_UNORM_SRGB;
    TextureFormat depth_stencil_format   = TextureFormat::D32_FLOAT;
    PrimitiveTopology primitive_topology = PrimitiveTopology::TRIANGLE_LIST;
    RID vertex_shader;
    RID pixel_shader;
    VertexLayout vert_layout;
    Span<const RID> resource_signatures;
    RasterizerStateDesc rasterizer_state;
    DepthStencilStateDesc depth_stencil_state;
    BlendStateDesc blend_state;
    MultisampleStateDesc multisample_state;
};

struct ComputePSODesc {
    String debug_name;
    RID compute_shader;
    Span<const RID> resource_signatures;
};

} // namespace nc::gfx

#pragma pop_macro( "OPAQUE" )
#pragma pop_macro( "NONE" )
