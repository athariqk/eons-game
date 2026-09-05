#include "material_template_importer.h"

#include <inicpp.h>

#include <ncore/resources/material_template.h>
#include <ncore/resources/shader.h>
#include <ncore/utils/log.h>

namespace nc {

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

    // [raster] / [multisample] removed — use // nc_pipeline: on the .slang file.
    if (ini_file.find( "raster" ) != ini_file.end() || ini_file.find( "multisample" ) != ini_file.end()) {
        NC_LOG_WARN_C(
            log::IO,
            "MaterialImporter: '{}' still has [raster]/[multisample]; ignored. "
            "Move policy to // nc_pipeline: in the shader.",
            path
        );
    }

    if (ini_file.find( "vertex" ) != ini_file.end()) {
        auto& vertex = ini_file["vertex"];
        if (vertex.find( "layout" ) != vertex.end())
            tmpl->vertex_layout_name = vertex["layout"].as<std::string>();
    }

    NC_LOG_INFO_C(
        log::IO, "MaterialImporter: '{}' -> shader surface_policy.from_shader={}", path,
        shader->get_surface_policy().from_shader
    );

    return tmpl;
}

} // namespace nc
