#pragma once

/**
 * Esoterica-style materials: no MaterialTemplate.
 *
 *   Shader              = code + optional // nc_pipeline: SurfacePolicy
 *   MaterialCreateDesc  = flags + vertex layout + debug name (at GPU create)
 *   MaterialComponent   = Shader Source RID + textures + param blob + overrides
 */

#include <cstring>

#include <ncore/resources/shader.h>
#include <ncore/resources/surface_policy.h>

namespace nc {

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

struct MaterialShaderParameterInfo {
    String name;
    uint32_t offset_bytes = 0;
    uint32_t stride_bytes = 0;
};

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

/** Arguments for RenderService::material_create (replaces MaterialTemplate). */
struct MaterialCreateDesc {
    String debug_name;
    String vertex_layout_name; // empty = reflect from VS
    MaterialShaderFlags flags = MaterialShaderFlags::None;
};

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

enum class MaterialDrawBucket : uint8_t { Opaque = 0, AlphaTest, AlphaBlend };

inline MaterialDrawBucket draw_bucket_from_flags( MaterialShaderFlags flags )
{
    if (any( flags & MaterialShaderFlags::AlphaBlend ))
        return MaterialDrawBucket::AlphaBlend;
    if (any( flags & MaterialShaderFlags::AlphaTest ))
        return MaterialDrawBucket::AlphaTest;
    return MaterialDrawBucket::Opaque;
}

} // namespace nc
