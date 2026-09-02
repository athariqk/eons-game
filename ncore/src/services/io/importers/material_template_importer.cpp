#include "material_template_importer.h"

#include <inicpp.h>

#include <ncore/resources/material_template.h>
#include <ncore/resources/shader.h>
#include <ncore/utils/log.h>

namespace nc {

static rhi::CullMode parse_cull_mode( const std::string& str )
{
    if (str == "None")
        return rhi::CullMode::NONE;
    if (str == "Front")
        return rhi::CullMode::FRONT;
    if (str == "Back")
        return rhi::CullMode::BACK;
    return rhi::CullMode::NONE;
}

static rhi::FillMode parse_fill_mode( const std::string& str )
{
    if (str == "Wireframe")
        return rhi::FillMode::WIREFRAME;
    return rhi::FillMode::SOLID;
}

static rhi::BlendPreset parse_blend( const std::string& str )
{
    if (str == "Opaque")
        return rhi::BlendPreset::OPAQUE;
    if (str == "AlphaPremultiplied")
        return rhi::BlendPreset::ALPHA_PREMULTIPLIED;
    if (str == "Additive")
        return rhi::BlendPreset::ADDITIVE;
    return rhi::BlendPreset::ALPHA_BLEND;
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

    Ref<Shader> shader;

    if (ini_file.find( "shader" ) != ini_file.end()) {
        auto& shaders = ini_file["shader"];

        if (shaders.find( "path" ) != shaders.end()) {
            String comp_path( shaders["path"].as<std::string>() );
            RID shader_handle = ctx.load( comp_path );
            shader            = ctx.get( shader_handle ).as<Shader>();
        }
    }

    if (!shader) {
        NC_LOG_ERROR_C( log::IO, "MaterialImporter: missing shader in '{}'", path );
        return nullptr;
    }

    tmpl->shader = shader;

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
