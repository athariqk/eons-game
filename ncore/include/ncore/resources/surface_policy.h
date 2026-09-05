#pragma once

/**
 * Pipeline policy split (replaces MaterialTemplate raster/blend/depth fields).
 *
 *   Shader  -> SurfacePolicy (from // nc_pipeline:)
 *   Instance -> MaterialSurfaceOverrides (wireframe, etc.)
 *   Pass    -> PassPipelineDefaults (depth/RT/force blend)
 */

#include <ncore/services/video/rhi_types.h>

namespace nc {

struct SurfacePolicy {
    rhi::CullMode cull_mode = rhi::CullMode::BACK;
    rhi::FillMode fill_mode = rhi::FillMode::SOLID;
    rhi::BlendPreset blend  = rhi::BlendPreset::OPAQUE;

    bool double_sided = false;

    /** Soft depth defaults when the pass does not force its own. */
    bool depth_test  = true;
    bool depth_write = true;

    enum class AlphaMode : uint8_t { Opaque = 0, Mask, Blend } alpha_mode = AlphaMode::Opaque;

    bool from_shader = false;
};

struct PassPipelineDefaults {
    bool force_depth                             = false;
    bool depth_test                              = true;
    bool depth_write                             = true;
    rhi::CompareFunc depth_func                  = rhi::CompareFunc::LESS_EQUAL;

    bool force_blend                             = false;
    rhi::BlendPreset blend                       = rhi::BlendPreset::OPAQUE;

    rhi::TextureFormat color_format              = rhi::TextureFormat::RGBA8_UNORM_SRGB;
    rhi::TextureFormat depth_format              = rhi::TextureFormat::D32_FLOAT;
    rhi::MultisampleStateDesc multisample        = { 1, 0 };
    rhi::PrimitiveTopology topology              = rhi::PrimitiveTopology::TRIANGLE_LIST;
    bool scissor                                 = true;
};

struct MaterialSurfaceOverrides {
    bool override_fill = false;
    rhi::FillMode fill_mode = rhi::FillMode::SOLID;

    bool override_cull = false;
    rhi::CullMode cull_mode = rhi::CullMode::BACK;

    bool override_blend = false;
    rhi::BlendPreset blend = rhi::BlendPreset::OPAQUE;
};

struct ResolvedSurfaceState {
    rhi::CullMode cull_mode = rhi::CullMode::BACK;
    rhi::FillMode fill_mode = rhi::FillMode::SOLID;
    rhi::BlendPreset blend  = rhi::BlendPreset::OPAQUE;
    bool depth_test         = true;
    bool depth_write        = true;
};

inline ResolvedSurfaceState resolve_surface(
    const SurfacePolicy& shader_defaults,
    const MaterialSurfaceOverrides& overrides = {},
    const PassPipelineDefaults& pass          = {}
)
{
    ResolvedSurfaceState out;
    out.cull_mode   = shader_defaults.double_sided ? rhi::CullMode::NONE : shader_defaults.cull_mode;
    out.fill_mode   = shader_defaults.fill_mode;
    out.blend       = shader_defaults.blend;
    out.depth_test  = shader_defaults.depth_test;
    out.depth_write = shader_defaults.depth_write;

    if (shader_defaults.alpha_mode == SurfacePolicy::AlphaMode::Blend &&
        out.blend == rhi::BlendPreset::OPAQUE) {
        out.blend = rhi::BlendPreset::ALPHA_BLEND;
    }

    if (overrides.override_fill)
        out.fill_mode = overrides.fill_mode;
    if (overrides.override_cull)
        out.cull_mode = overrides.cull_mode;
    if (overrides.override_blend)
        out.blend = overrides.blend;

    if (pass.force_depth) {
        out.depth_test  = pass.depth_test;
        out.depth_write = pass.depth_write;
    }
    if (pass.force_blend)
        out.blend = pass.blend;

    return out;
}

/** Encode into RenderStorage::PSOFlags bit layout. */
inline uint64_t encode_pso_flags(
    const ResolvedSurfaceState& surface,
    const PassPipelineDefaults& pass
)
{
    constexpr uint64_t PSO_CULL_SHIFT          = 0;
    constexpr uint64_t PSO_DEPTH_TEST         = 1ull << 2;
    constexpr uint64_t PSO_DEPTH_WRITE        = 1ull << 3;
    constexpr uint64_t PSO_BLEND_SHIFT        = 4;
    constexpr uint64_t PSO_TOPOLOGY_SHIFT     = 8;
    constexpr uint64_t PSO_RT_FMT_SHIFT       = 12;
    constexpr uint64_t PSO_DST_FMT_SHIFT      = 15;
    constexpr uint64_t PSO_MSAA_COUNT_SHIFT   = 19;
    constexpr uint64_t PSO_MSAA_QUALITY_SHIFT = 23;
    constexpr uint64_t PSO_SCISSOR            = 1ull << 27;
    constexpr uint64_t PSO_FILL_SHIFT         = 28;

    uint64_t flags = 0;
    flags |= ( static_cast<uint64_t>( surface.cull_mode ) & 3u ) << PSO_CULL_SHIFT;
    flags |= ( static_cast<uint64_t>( surface.fill_mode ) & 3u ) << PSO_FILL_SHIFT;
    flags |= ( static_cast<uint64_t>( surface.blend ) & 15u ) << PSO_BLEND_SHIFT;

    if (surface.depth_test)
        flags |= PSO_DEPTH_TEST;
    if (surface.depth_write)
        flags |= PSO_DEPTH_WRITE;

    flags |= ( static_cast<uint64_t>( pass.topology ) & 15u ) << PSO_TOPOLOGY_SHIFT;
    flags |= ( static_cast<uint64_t>( pass.color_format ) & 7u ) << PSO_RT_FMT_SHIFT;

    const bool need_depth = surface.depth_test || surface.depth_write;
    const auto dst_fmt =
        need_depth ? pass.depth_format : rhi::TextureFormat::UNKNOWN;
    flags |= ( static_cast<uint64_t>( dst_fmt ) & 15u ) << PSO_DST_FMT_SHIFT;

    flags |= ( static_cast<uint64_t>( pass.multisample.count ) & 15u ) << PSO_MSAA_COUNT_SHIFT;
    flags |= ( static_cast<uint64_t>( pass.multisample.quality ) & 15u ) << PSO_MSAA_QUALITY_SHIFT;

    if (pass.scissor)
        flags |= PSO_SCISSOR;

    return flags;
}

} // namespace nc
