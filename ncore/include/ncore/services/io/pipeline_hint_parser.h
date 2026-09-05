#pragma once

#include <cctype>
#include <string_view>

#include <ncore/resources/surface_policy.h>

namespace nc {

namespace pipeline_hint_detail {

inline std::string_view trim( std::string_view s )
{
    while (!s.empty() && std::isspace( static_cast<unsigned char>( s.front() ) ))
        s.remove_prefix( 1 );
    while (!s.empty() && std::isspace( static_cast<unsigned char>( s.back() ) ))
        s.remove_suffix( 1 );
    return s;
}

inline bool eq_i( std::string_view a, std::string_view b )
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower( static_cast<unsigned char>( a[i] ) ) !=
            std::tolower( static_cast<unsigned char>( b[i] ) ))
            return false;
    }
    return true;
}

inline void apply_kv( SurfacePolicy& out, std::string_view key, std::string_view val )
{
    key = trim( key );
    val = trim( val );

    if (eq_i( key, "cull" )) {
        if (eq_i( val, "none" ))
            out.cull_mode = rhi::CullMode::NONE;
        else if (eq_i( val, "front" ))
            out.cull_mode = rhi::CullMode::FRONT;
        else if (eq_i( val, "back" ))
            out.cull_mode = rhi::CullMode::BACK;
    } else if (eq_i( key, "fill" )) {
        if (eq_i( val, "solid" ))
            out.fill_mode = rhi::FillMode::SOLID;
        else if (eq_i( val, "wireframe" ))
            out.fill_mode = rhi::FillMode::WIREFRAME;
    } else if (eq_i( key, "blend" )) {
        if (eq_i( val, "opaque" ))
            out.blend = rhi::BlendPreset::OPAQUE;
        else if (eq_i( val, "alpha" ))
            out.blend = rhi::BlendPreset::ALPHA_BLEND;
        else if (eq_i( val, "premul" ) || eq_i( val, "premultiplied" ))
            out.blend = rhi::BlendPreset::ALPHA_PREMULTIPLIED;
        else if (eq_i( val, "additive" ))
            out.blend = rhi::BlendPreset::ADDITIVE;
    } else if (eq_i( key, "alpha" )) {
        if (eq_i( val, "opaque" ))
            out.alpha_mode = SurfacePolicy::AlphaMode::Opaque;
        else if (eq_i( val, "mask" ))
            out.alpha_mode = SurfacePolicy::AlphaMode::Mask;
        else if (eq_i( val, "blend" )) {
            out.alpha_mode = SurfacePolicy::AlphaMode::Blend;
            if (out.blend == rhi::BlendPreset::OPAQUE)
                out.blend = rhi::BlendPreset::ALPHA_BLEND;
        }
    } else if (eq_i( key, "double_sided" ) || eq_i( key, "doublesided" )) {
        out.double_sided = eq_i( val, "true" ) || eq_i( val, "1" ) || eq_i( val, "yes" );
    } else if (eq_i( key, "depth_test" )) {
        out.depth_test = eq_i( val, "true" ) || eq_i( val, "1" ) || eq_i( val, "yes" );
    } else if (eq_i( key, "depth_write" )) {
        out.depth_write = eq_i( val, "true" ) || eq_i( val, "1" ) || eq_i( val, "yes" );
    }
}

} // namespace pipeline_hint_detail

/** Last `// nc_pipeline:` line in source wins. Returns true if found. */
inline bool parse_pipeline_hint_from_source( std::string_view source, SurfacePolicy& out )
{
    using namespace pipeline_hint_detail;

    constexpr std::string_view kTag = "nc_pipeline:";
    bool found                      = false;
    SurfacePolicy policy;

    size_t line_start = 0;
    while (line_start < source.size()) {
        size_t line_end = source.find( '\n', line_start );
        if (line_end == std::string_view::npos)
            line_end = source.size();

        std::string_view line = source.substr( line_start, line_end - line_start );
        line_start            = line_end + 1;

        line = trim( line );
        if (line.size() < 2 || line[0] != '/' || line[1] != '/')
            continue;

        line     = trim( line.substr( 2 ) );
        auto pos = line.find( kTag );
        if (pos == std::string_view::npos)
            continue;

        found              = true;
        policy             = SurfacePolicy{};
        policy.from_shader = true;

        std::string_view body = trim( line.substr( pos + kTag.size() ) );
        while (!body.empty()) {
            size_t comma = body.find( ',' );
            std::string_view pair =
                trim( comma == std::string_view::npos ? body : body.substr( 0, comma ) );
            body = comma == std::string_view::npos ? std::string_view{} : body.substr( comma + 1 );

            size_t eq = pair.find( '=' );
            if (eq == std::string_view::npos)
                continue;
            apply_kv( policy, pair.substr( 0, eq ), pair.substr( eq + 1 ) );
        }
    }

    if (found)
        out = policy;
    return found;
}

} // namespace nc
