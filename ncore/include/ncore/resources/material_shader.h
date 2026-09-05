#pragma once

/**
 * Esoterica-inspired material shader model for ncore.
 *
 * MaterialShader = compiled Shader + surface flags + reflected parameter layout.
 * Material (MaterialTemplate) = shader reference + instance parameter values.
 * PSO / buckets = pass + flags, not authored on the material asset.
 *
 * See docs/materials_esoterica.md
 */

#include <cstring>

#include <ncore/resources/shader.h>
#include <ncore/resources/surface_policy.h>

namespace nc {

/** Surface / batching flags (Esoterica MaterialShaderFlags). */
enum class MaterialShaderFlags : uint32_t {
    None       = 0,
    AlphaTest  = 1u << 0,
    AlphaBlend = 1u << 1,
    TwoSided   = 1u << 2,
};

constexpr MaterialShaderFlags operator|( MaterialShaderFlags a, MaterialShaderFlags b )
{
    return static_cast<MaterialShaderFlags>( static_cast<uint32_t>( a ) | static_cast<uint32_t>( b ) );
}
constexpr MaterialShaderFlags operator&( MaterialShaderFlags a, MaterialShaderFlags b )
{
    return static_cast<MaterialShaderFlags>( static_cast<uint32_t>( a ) & static_cast<uint32_t>( b ) );
}
constexpr bool any( MaterialShaderFlags f )
{
    return static_cast<uint32_t>( f ) != 0;
}

/** Reflected material parameter (Esoterica MaterialShaderParameterInfo). */
struct MaterialShaderParameterInfo {
    String name;
    uint32_t offset_bytes = 0;
    uint32_t stride_bytes = 0;
};

/**
 * Map flags into SurfacePolicy (fill depth/blend/cull soft defaults).
 * Existing SurfacePolicy fields from // nc_pipeline: are used as a base when provided.
 */
inline SurfacePolicy surface_policy_from_flags( MaterialShaderFlags flags, SurfacePolicy base = {} )
{
    SurfacePolicy p = base;
    if (any( flags & MaterialShaderFlags::TwoSided ))
        p.double_sided = true;
    if (any( flags & MaterialShaderFlags::AlphaBlend )) {
        p.alpha_mode = SurfacePolicy::AlphaMode::Blend;
        if (p.blend == rhi::BlendPreset::OPAQUE)
            p.blend = rhi::BlendPreset::ALPHA_BLEND;
    }
    if (any( flags & MaterialShaderFlags::AlphaTest )) {
        p.alpha_mode = SurfacePolicy::AlphaMode::Mask;
        if (!any( flags & MaterialShaderFlags::AlphaBlend ))
            p.blend = rhi::BlendPreset::OPAQUE;
    }
    p.from_shader = true;
    return p;
}

inline MaterialShaderFlags flags_from_surface_policy( const SurfacePolicy& p )
{
    MaterialShaderFlags f = MaterialShaderFlags::None;
    if (p.double_sided || p.cull_mode == rhi::CullMode::NONE)
        f = f | MaterialShaderFlags::TwoSided;
    if (p.alpha_mode == SurfacePolicy::AlphaMode::Mask)
        f = f | MaterialShaderFlags::AlphaTest;
    if (p.alpha_mode == SurfacePolicy::AlphaMode::Blend || p.blend == rhi::BlendPreset::ALPHA_BLEND ||
        p.blend == rhi::BlendPreset::ALPHA_PREMULTIPLIED || p.blend == rhi::BlendPreset::ADDITIVE)
        f = f | MaterialShaderFlags::AlphaBlend;
    return f;
}

/**
 * Runtime view of a graphics Shader used as a material program.
 * Does not own the Shader resource — holds a Ref and derived metadata.
 */
struct MaterialShaderDesc {
    Ref<Shader> shader;
    MaterialShaderFlags flags = MaterialShaderFlags::None;
    String parameters_struct_name; // e.g. "MaterialParams"
    DynamicArray<MaterialShaderParameterInfo> parameters;

    SurfacePolicy surface_policy() const
    {
        SurfacePolicy from_shader = shader ? shader->get_surface_policy() : SurfacePolicy{};
        if (flags != MaterialShaderFlags::None)
            return surface_policy_from_flags( flags, from_shader );
        return from_shader;
    }
};

/**
 * Instance parameter blob (Esoterica MaterialShaderParametersInstance lite).
 * CPU-side packing; GPU upload via material_set_params.
 */
struct MaterialParameterStorage {
    static constexpr uint32_t kMaxBytes = 256;

    alignas( 16 ) uint8_t bytes[kMaxBytes]{};
    uint32_t size = 0;

    void clear()
    {
        size = 0;
        std::memset( bytes, 0, sizeof( bytes ) );
    }

    template <typename T>
    void set_at( uint32_t offset, const T& value )
    {
        static_assert( sizeof( T ) <= kMaxBytes );
        NC_VERIFY( offset + sizeof( T ) <= kMaxBytes );
        std::memcpy( bytes + offset, &value, sizeof( T ) );
        if (offset + sizeof( T ) > size)
            size = offset + static_cast<uint32_t>( sizeof( T ) );
    }
};

/** Draw bucket kind (Esoterica shader bucket subset). */
enum class MaterialDrawBucket : uint8_t {
    Opaque = 0,
    AlphaTest,
    AlphaBlend,
};

inline MaterialDrawBucket draw_bucket_from_flags( MaterialShaderFlags flags )
{
    if (any( flags & MaterialShaderFlags::AlphaBlend ))
        return MaterialDrawBucket::AlphaBlend;
    if (any( flags & MaterialShaderFlags::AlphaTest ))
        return MaterialDrawBucket::AlphaTest;
    return MaterialDrawBucket::Opaque;
}

} // namespace nc
