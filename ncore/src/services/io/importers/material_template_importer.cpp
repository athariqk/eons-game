#include "material_template_importer.h"

#include <inicpp.h>

#include <ncore/resources/material_template.h>
#include <ncore/resources/shader.h>
#include <ncore/utils/log.h>

namespace nc {

static CullMode parse_cull_mode( const std::string& str )
{
    if (str == "None")
        return CullMode::NONE;
    if (str == "Front")
        return CullMode::FRONT;
    if (str == "Back")
        return CullMode::BACK;
    return CullMode::NONE;
}

static FillMode parse_fill_mode( const std::string& str )
{
    if (str == "Wireframe")
        return FillMode::WIREFRAME;
    return FillMode::SOLID;
}

static BlendPreset parse_blend( const std::string& str )
{
    if (str == "Opaque")
        return BlendPreset::OPAQUE;
    if (str == "AlphaPremultiplied")
        return BlendPreset::ALPHA_PREMULTIPLIED;
    if (str == "Additive")
        return BlendPreset::ADDITIVE;
    return BlendPreset::ALPHA_BLEND;
}

Ref<IResource> MaterialImporter::import( const String& path, Context ctx )
{
    ini::IniFile ini_file;
    try {
        ini_file.load( path.c_str() );
    } catch (const std::exception& e) {
        NC_LOG_ERROR_C( log::IO, "MaterialImporter: failed to parse '{}': {}", path, e.what() );
        return nullptr;
    }

    auto tmpl = Ref<MaterialTemplate>::create();

    if (ini_file.find( "header" ) != ini_file.end()) {
        auto& header = ini_file["header"];
        if (header.find( "debug_name" ) != header.end())
            tmpl->debug_name = header["debug_name"].as<std::string>();
        else
            tmpl->debug_name = path;
    } else {
        tmpl->debug_name = path;
    }

    Ref<Shader> vs_shader;
    Ref<Shader> ps_shader;

    if (ini_file.find( "shaders" ) != ini_file.end()) {
        auto& shaders = ini_file["shaders"];

        // Prefer single composite shader file over separate vs/ps
        if (shaders.find( "composite" ) != shaders.end()) {
            String comp_path( shaders["composite"].as<std::string>() );
            RID comp_rid     = ctx.load( comp_path );
            auto comp_raw    = ctx.get( comp_rid );
            auto comp_shader = comp_raw.as<CompositeShader>();
            if (comp_shader) {
                vs_shader = comp_shader->get_shader( ShaderType::VERTEX );
                ps_shader = comp_shader->get_shader( ShaderType::PIXEL );
            }
        } else {
            if (shaders.find( "vs" ) != shaders.end()) {
                String vs_path( shaders["vs"].as<std::string>() );
                RID vs_rid        = ctx.load( vs_path );
                auto vs_raw       = ctx.get( vs_rid );
                auto vs_composite = vs_raw.as<CompositeShader>();
                if (vs_composite)
                    vs_shader = vs_composite->get_shader( ShaderType::VERTEX );
            }

            if (shaders.find( "ps" ) != shaders.end()) {
                String ps_path( shaders["ps"].as<std::string>() );
                RID ps_rid        = ctx.load( ps_path );
                auto ps_raw       = ctx.get( ps_rid );
                auto ps_composite = ps_raw.as<CompositeShader>();
                if (ps_composite)
                    ps_shader = ps_composite->get_shader( ShaderType::PIXEL );
            }
        }
    }

    if (!vs_shader || !ps_shader) {
        NC_LOG_ERROR_C( log::IO, "MaterialImporter: missing shaders in '{}'", path );
        return nullptr;
    }

    tmpl->vs = vs_shader;
    tmpl->ps = ps_shader;

    for (const auto& p : vs_shader->get_desc().params) {
        tmpl->params.push_back( p );
    }

    for (const auto& p : ps_shader->get_desc().params) {
        bool duplicate = false;
        for (const auto& existing : tmpl->params) {
            if (existing.name == p.name && existing.binding_space == p.binding_space &&
                existing.binding_idx == p.binding_idx) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
            tmpl->params.push_back( p );
    }

    if (ini_file.find( "raster" ) != ini_file.end()) {
        auto& raster = ini_file["raster"];

        if (raster.find( "cull_mode" ) != raster.end())
            tmpl->cull_mode = parse_cull_mode( raster["cull_mode"].as<std::string>() );

        if (raster.find( "fill_mode" ) != raster.end())
            tmpl->fill_mode = parse_fill_mode( raster["fill_mode"].as<std::string>() );

        if (raster.find( "depth_test" ) != raster.end())
            tmpl->depth_test = raster["depth_test"].as<bool>();

        if (raster.find( "depth_write" ) != raster.end())
            tmpl->depth_write = raster["depth_write"].as<bool>();

        if (raster.find( "blend" ) != raster.end())
            tmpl->blend = parse_blend( raster["blend"].as<std::string>() );
    }

    if (ini_file.find( "multisample" ) != ini_file.end()) {
        auto& ms = ini_file["multisample"];

        if (ms.find( "count" ) != ms.end())
            tmpl->multisample_state.count = static_cast<uint8_t>( ms["count"].as<int>() );

        if (ms.find( "quality" ) != ms.end())
            tmpl->multisample_state.quality = static_cast<uint8_t>( ms["quality"].as<int>() );
    }

    if (ini_file.find( "vertex" ) != ini_file.end()) {
        auto& vertex = ini_file["vertex"];
        if (vertex.find( "layout" ) != vertex.end())
            tmpl->vertex_layout_name = vertex["layout"].as<std::string>();
    }

    return tmpl.as<IResource>();
}

} // namespace nc
