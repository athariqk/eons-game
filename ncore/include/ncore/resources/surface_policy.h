#pragma once

/**
 * Materials refactor prototype (branch grok/materials-refactor).
 *
 * Split pipeline concerns the way most engines do in practice:
 *
 *   Shader  -> SurfacePolicy defaults (optional, from // nc_pipeline: or attrs)
 *   Material instance -> small surface overrides (double-sided, wireframe, alpha)
 *   Render pass -> depth / RT / most blend behavior for this draw
 *
 * MaterialTemplate's full PSO field list is the old combined model; new code
 * should compose SurfacePolicy + PassPipelineDefaults into PSOFlags instead.
 */

#include <ncore/services/video/rhi_types.h>

namespace nc {

/**
 * Surface-facing policy. Small, material/shader scoped — not a full PSO.
 * Passes may override blend/depth for depth-prepass, shadows, etc.
 */
struct SurfacePolicy {
    rhi::CullMode cull_mode   = rhi::CullMode::BACK;
    rhi::FillMode fill_mode   = rhi::FillMode::SOLID;
    rhi::BlendPreset blend    = rhi::BlendPreset::OPAQUE;

    /** When true, geometry is rendered two-sided (cull = NONE). */
    bool double_sided         = false;

    /**
     * Soft alpha mode (Godot-style). Pass may still force opaque for shadow/
     * depth passes.
     */
    enum class AlphaMode : uint8_t {
        Opaque = 0,
        Mask,   // alpha test (future)
        Blend,  // transparent
    } alpha_mode              = AlphaMode::Opaque;

    /** True if this policy was parsed from shader source (vs engine default). */
    bool from_shader          = false;
};

/**
 * Defaults owned by a render pass / draw list (frame graph).
 * Wins over material for depth and often for blend in special passes.
 */
struct PassPipelineDefaults {
    bool depth_test                              = true;
    bool depth_write                             = true;
    rhi::CompareFunc depth_func                  = rhi::CompareFunc::LESS_EQUAL;

    /** If set, forces blend for this pass (e.g. opaque for depth prepass). */
    bool force_blend                             = false;
    rhi::BlendPreset blend                       = rhi::BlendPreset::OPAQUE;

    rhi::TextureFormat color_format              = rhi::TextureFormat::RGBA8_UNORM_SRGB;
    rhi::TextureFormat depth_format              = rhi::TextureFormat::D32_FLOAT;
    rhi::MultisampleStateDesc multisample        = { 1, 0 };
    rhi::PrimitiveTopology topology              = rhi::PrimitiveTopology::TRIANGLE_LIST;
};

/**
 * Runtime overrides from MaterialComponent (debug wireframe, etc.).
 * Only fields that are intentionally instance-mutable.
 */
struct MaterialSurfaceOverrides {
    bool override_fill = false;
    rhi::FillMode fill_mode = rhi::FillMode::SOLID;

    bool override_cull = false;
    rhi::CullMode cull_mode = rhi::CullMode::BACK;

    bool override_blend = false;
    rhi::BlendPreset blend = rhi::BlendPreset::OPAQUE;
};

/**
 * Resolved surface after shader defaults + instance overrides.
 * Depth is intentionally absent — that stays on the pass.
 */
struct ResolvedSurfaceState {
    rhi::CullMode cull_mode = rhi::CullMode::BACK;
    rhi::FillMode fill_mode = rhi::FillMode::SOLID;
    rhi::BlendPreset blend  = rhi::BlendPreset::OPAQUE;
};

inline ResolvedSurfaceState resolve_surface(
    const SurfacePolicy& shader_defaults,
    const MaterialSurfaceOverrides& overrides = {}
)
{
    ResolvedSurfaceState out;
    out.cull_mode = shader_defaults.double_sided ? rhi::CullMode::NONE : shader_defaults.cull_mode;
    out.fill_mode = shader_defaults.fill_mode;
    out.blend     = shader_defaults.blend;

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

    return out;
}

/**
 * Encode surface + pass into the same bit layout as RenderStorage::PSOFlags.
 * Mirrors shifts in render_storage.h so prototype keys stay compatible.
 */
inline uint64_t encode_pso_flags(
    const ResolvedSurfaceState& surface,
    const PassPipelineDefaults& pass
)
{
    // Must match RenderStorage::PSOFlags shifts.
    constexpr uint64_t PSO_CULL_SHIFT       = 0;
    constexpr uint64_t PSO_DEPTH_TEST      = 1ull << 2;
    constexpr uint64_t PSO_DEPTH_WRITE     = 1ull << 3;
    constexpr uint64_t PSO_BLEND_SHIFT     = 4;
    constexpr uint64_t PSO_TOPOLOGY_SHIFT  = 8;
    constexpr uint64_t PSO_RT_FMT_SHIFT    = 12;
    constexpr uint64_t PSO_DST_FMT_SHIFT   = 15;
    constexpr uint64_t PSO_MSAA_COUNT_SHIFT = 19;
    constexpr uint64_t PSO_MSAA_QUALITY_SHIFT = 23;
    constexpr uint64_t PSO_FILL_SHIFT      = 28;

    uint64_t flags = 0;

    flags |= ( static_cast<uint64_t>( surface.cull_mode ) & 3u ) << PSO_CULL_SHIFT;
    flags |= ( static_cast<uint64_t>( surface.fill_mode ) & 3u ) << PSO_FILL_SHIFT;

    const rhi::BlendPreset blend = pass.force_blend ? pass.blend : surface.blend;
    flags |= ( static_cast<uint64_t>( blend ) & 15u ) << PSO_BLEND_SHIFT;

    if (pass.depth_test)
        flags |= PSO_DEPTH_TEST;
    if (pass.depth_write)
        flags |= PSO_DEPTH_WRITE;

    flags |= ( static_cast<uint64_t>( pass.topology ) & 15u ) << PSO_TOPOLOGY_SHIFT;
    flags |= ( static_cast<uint64_t>( pass.color_format ) & 7u ) << PSO_RT_FMT_SHIFT;
    flags |= ( static_cast<uint64_t>( pass.depth_format ) & 15u ) << PSO_DST_FMT_SHIFT;
    flags |= ( static_cast<uint64_t>( pass.multisample.count ) & 15u ) << PSO_MSAA_COUNT_SHIFT;
    flags |= ( static_cast<uint64_t>( pass.multisample.quality ) & 15u ) << PSO_MSAA_QUALITY_SHIFT;

    return flags;
}

/**
 * Build MaterialTemplate-compatible fields from composed policy.
 * Bridge for gradual migration while material_create still takes MaterialTemplate.
 */
inline void apply_to_legacy_template_fields(
    const ResolvedSurfaceState& surface,
    const PassPipelineDefaults& pass,
    rhi::CullMode& cull_mode,
    rhi::FillMode& fill_mode,
    bool& depth_test,
    bool& depth_write,
    rhi::BlendPreset& blend,
    rhi::MultisampleStateDesc& multisample
)
{
    cull_mode    = surface.cull_mode;
    fill_mode    = surface.fill_mode;
    depth_test   = pass.depth_test;
    depth_write  = pass.depth_write;
    blend        = pass.force_blend ? pass.blend : surface.blend;
    multisample  = pass.multisample;
}

} // namespace nc
