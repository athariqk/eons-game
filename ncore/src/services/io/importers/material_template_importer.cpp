#include "material_template_importer.h"

#include <cctype>

#include <inicpp.h>

#include <ncore/resources/material_shader.h>
#include <ncore/resources/material_template.h>
#include <ncore/resources/shader.h>
#include <ncore/utils/log.h>

namespace nc {

static MaterialShaderFlags parse_flags( const std::string& str )
{
    MaterialShaderFlags f = MaterialShaderFlags::None;
    size_t start          = 0;
    while (start < str.size()) {
        size_t comma    = str.find( ',', start );
        std::string tok = str.substr( start, comma == std::string::npos ? std::string::npos : comma - start );
        while (!tok.empty() && std::isspace( static_cast<unsigned char>( tok.front() ) ))
            tok.erase( tok.begin() );
        while (!tok.empty() && std::isspace( static_cast<unsigned char>( tok.back() ) ))
            tok.pop_back();
        if (tok == "AlphaTest")
            f = f | MaterialShaderFlags::AlphaTest;
        else if (tok == "AlphaBlend")
            f = f | MaterialShaderFlags::AlphaBlend;
        else if (tok == "TwoSided")
            f = f | MaterialShaderFlags::TwoSided;
        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
    return f;
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

    if (ini_file.find( "flags" ) != ini_file.end()) {
        auto& fl = ini_file["flags"];
        if (fl.find( "surface" ) != fl.end())
            tmpl->flags = parse_flags( fl["surface"].as<std::string>() );
    }

    if (ini_file.find( "raster" ) != ini_file.end() || ini_file.find( "multisample" ) != ini_file.end()) {
        NC_LOG_WARN_C(
            log::IO,
            "MaterialImporter: '{}' [raster]/[multisample] ignored. "
            "Use // nc_pipeline: on the shader or [flags] surface=TwoSided,AlphaBlend",
            path
        );
    }

    if (ini_file.find( "vertex" ) != ini_file.end()) {
        auto& vertex = ini_file["vertex"];
        if (vertex.find( "layout" ) != vertex.end())
            tmpl->vertex_layout_name = vertex["layout"].as<std::string>();
    }

    if (tmpl->flags == MaterialShaderFlags::None)
        tmpl->flags = flags_from_surface_policy( shader->get_surface_policy() );

    NC_LOG_INFO_C(
        log::IO, "MaterialImporter: '{}' shader ok flags=0x{:x} bucket={}", path,
        static_cast<uint32_t>( tmpl->flags ), static_cast<int>( draw_bucket_from_flags( tmpl->flags ) )
    );

    return tmpl;
}

} // namespace nc
