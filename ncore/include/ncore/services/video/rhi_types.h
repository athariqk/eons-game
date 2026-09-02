#pragma once

#include <span>
#include <string>

#include <ncore/core/collection.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/utils/enum.h>

#pragma push_macro( "NONE" )
#pragma push_macro( "OPAQUE" )
#undef NONE
#undef OPAQUE

namespace nc::rhi {

enum class PrimitiveTopology {
    TRIANGLE_LIST,
    POINT_LIST,
};

enum class FillMode {
    SOLID,
    WIREFRAME
};
NENUM( FillMode, NENUM_ELEMENT( FillMode, SOLID ), NENUM_ELEMENT( FillMode, WIREFRAME ) )

enum class ResourceBindFlags : uint32_t {
    NONE               = 0,
    VERTEX_BUFFER      = 1 << 0,
    INDEX_BUFFER       = 1 << 1,
    UNIFORM_BUFFER     = 1 << 2,
    SHADER_RESOURCE    = 1 << 3,
    STREAM_OUTPUT      = 1 << 4,
    RENDER_TARGET      = 1 << 5,
    DEPTH_STENCIL      = 1 << 6,
    UNORDERED_ACCESS   = 1 << 7,
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
    IMMUTABLE = 0,
    DEFAULT,
    DYNAMIC,
    STAGING,
    UNIFIED,
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

enum class ResourceAccessFlags : uint8_t {
    NONE       = 0,
    READ       = 1 << 0,
    WRITE      = 1 << 1,
    READ_WRITE = READ | WRITE
};
ENABLE_BITMASK( ResourceAccessFlags )
NENUM(
    ResourceAccessFlags, NENUM_ELEMENT( ResourceAccessFlags, NONE ), NENUM_ELEMENT( ResourceAccessFlags, READ ),
    NENUM_ELEMENT( ResourceAccessFlags, WRITE ), NENUM_ELEMENT( ResourceAccessFlags, READ_WRITE )
)

enum class TextureFormat {
    RGBA8_UNORM,
    RGBA8_UNORM_SRGB,
    R32_FLOAT,
    RG32_FLOAT,
    RGBA32_FLOAT,
    D32_FLOAT,
    UNKNOWN,
};
NENUM(
    TextureFormat, NENUM_ELEMENT( TextureFormat, RGBA8_UNORM ), NENUM_ELEMENT( TextureFormat, RGBA8_UNORM_SRGB ),
    NENUM_ELEMENT( TextureFormat, R32_FLOAT ), NENUM_ELEMENT( TextureFormat, RG32_FLOAT ),
    NENUM_ELEMENT( TextureFormat, RGBA32_FLOAT ), NENUM_ELEMENT( TextureFormat, D32_FLOAT ),
    NENUM_ELEMENT( TextureFormat, UNKNOWN )
)

enum class ResourceDimension {
    DIM_1D,
    DIM_2D,
    DIM_3D,
    DIM_CUBE,
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

enum class ResourceType {
    UNKNOWN,
    CONSTANT_BUFFER,
    TEXTURE_SRV,
    BUFFER_SRV,
    TEXTURE_UAV,
    BUFFER_UAV,
    SAMPLER,
    VARYING_INPUT,
};
NENUM(
    ResourceType, NENUM_ELEMENT( ResourceType, UNKNOWN ), NENUM_ELEMENT( ResourceType, CONSTANT_BUFFER ),
    NENUM_ELEMENT( ResourceType, TEXTURE_SRV ), NENUM_ELEMENT( ResourceType, BUFFER_SRV ),
    NENUM_ELEMENT( ResourceType, TEXTURE_UAV ), NENUM_ELEMENT( ResourceType, BUFFER_UAV ),
    NENUM_ELEMENT( ResourceType, SAMPLER ), NENUM_ELEMENT( ResourceType, VARYING_INPUT ),
)

enum class ResourceVarType {
    STATIC,
    MUTABLE,
    DYNAMIC,
};

enum class ResourceFlags {
    NONE               = 0,
    NO_DYNAMIC_BUFFERS = 1,
    COMBINED_SAMPLER   = 2,
    FORMATTED_BUFFER   = 3,
};

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
    SHADER_RESOURCE,
    UNORDERED_ACCESS
};
NENUM(
    BufferViewType, NENUM_ELEMENT( BufferViewType, SHADER_RESOURCE ), NENUM_ELEMENT( BufferViewType, UNORDERED_ACCESS )
)

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

enum ShaderValueType : uint16_t {
    FLOAT            = 1 << 0,
    FLOAT2           = 1 << 1,
    FLOAT3           = 1 << 2,
    FLOAT4           = 1 << 3,
    INT              = 1 << 4,
    INT2             = 1 << 5,
    INT3             = 1 << 6,
    INT4             = 1 << 7,
    USHORT4          = 1 << 8,
    MAT4             = 1 << 9,
    BOOL             = 1 << 10,
    UBYTE4_NORM      = 1 << 11,
    TEXTURE_2D       = 1 << 12,
    TEXTURE_CUBED    = 1 << 13,
    TEXTURE_2D_ARRAY = 1 << 14,
    SAMPLER          = 1 << 15,
    UNKNOWN
};
ENABLE_BITMASK( ShaderValueType )
NENUM(
    ShaderValueType, NENUM_ELEMENT( ShaderValueType, FLOAT ), NENUM_ELEMENT( ShaderValueType, FLOAT2 ),
    NENUM_ELEMENT( ShaderValueType, FLOAT3 ), NENUM_ELEMENT( ShaderValueType, FLOAT4 ),
    NENUM_ELEMENT( ShaderValueType, INT ), NENUM_ELEMENT( ShaderValueType, INT2 ),
    NENUM_ELEMENT( ShaderValueType, INT3 ), NENUM_ELEMENT( ShaderValueType, INT4 ),
    NENUM_ELEMENT( ShaderValueType, USHORT4 ), NENUM_ELEMENT( ShaderValueType, MAT4 ),
    NENUM_ELEMENT( ShaderValueType, BOOL ), NENUM_ELEMENT( ShaderValueType, UBYTE4_NORM ),
    NENUM_ELEMENT( ShaderValueType, TEXTURE_2D ), NENUM_ELEMENT( ShaderValueType, TEXTURE_CUBED ),
    NENUM_ELEMENT( ShaderValueType, TEXTURE_2D_ARRAY ), NENUM_ELEMENT( ShaderValueType, SAMPLER ),
    NENUM_ELEMENT( ShaderValueType, UNKNOWN )
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

struct PipelineResourceDesc {
    String name;
    ShaderStage stage          = ShaderStage::NONE;
    ResourceType resource_type = ResourceType::CONSTANT_BUFFER;
    ResourceVarType var_type = ResourceVarType::DYNAMIC;
    ResourceFlags flags = ResourceFlags::NONE;
    uint32_t array_size = 1;
};

typedef uint8_t SetIndex;

struct ResourceSignatureDesc {
    String name;
    SetIndex set_idx = 0;
    DynamicArray<PipelineResourceDesc> resources;
};

using ResourceLayoutDesc = HashMap<SetIndex, ResourceSignatureDesc>;

struct ResourceMappingEntry {
    const char* variable_name;
    ResourceType kind;
    RID resource;
};

struct BufferDesc {
    String debug_name;
    ResourceBindFlags bind_mask;
    size_t size;
    const void* initial_data        = nullptr;
    ResourceUsage usage             = ResourceUsage::DEFAULT;
    ResourceAccessFlags access_mask = ResourceAccessFlags::NONE;
    BufferMode mode                 = BufferMode::RAW;
    /** Byte size of one element. Required when mode is STRUCTURED or FORMATTED. */
    size_t element_stride           = 0;
};

struct TextureDataDesc {
    const void* pixels = nullptr;
};

struct TextureDesc {
    String debug_name;
    TextureFormat format            = TextureFormat::RGBA8_UNORM_SRGB;
    ResourceDimension dimension     = ResourceDimension::DIM_2D;
    ResourceUsage usage             = ResourceUsage::DEFAULT;
    ResourceBindFlags bind_mask     = ResourceBindFlags::SHADER_RESOURCE;
    ResourceAccessFlags access_mask = ResourceAccessFlags::NONE;
    uint32_t width                  = 0;
    uint32_t height                 = 0;
    uint32_t array_size             = 1;
    uint32_t mip_levels             = 1;
    uint32_t sample_count           = 1;
    DynamicArray<TextureDataDesc> subresources;
};

struct VertexLayoutElement {
    uint32_t location;
    ShaderValueType type;
    uint32_t buffer_slot     = 0;
    size_t stride            = 0;
    uint32_t relative_offset = ~0u;
    bool normalized          = false;
    VertexFrequency frequency;
    uint32_t instance_step_rate = 1;
    const char* hlsl_semantic   = "ATTRIB";
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

struct ShaderCreateDesc {
    StringView name;
    rhi::ShaderStage stage;
    Span<const uint32_t> bytecode;
};

struct GraphicsPSODesc {
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

} // namespace nc::rhi

#pragma pop_macro( "OPAQUE" )
#pragma pop_macro( "NONE" )
