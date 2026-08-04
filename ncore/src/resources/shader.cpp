#include <ncore/resources/shader.h>

namespace nc {

Shader::Shader( const ShaderDesc& p_desc ) : desc( p_desc ) {}

ResourceFormatID Shader::get_format_id() const
{
    return "shad";
}

ShaderType Shader::get_stage() const
{
    return desc.stage;
}

std::string_view Shader::get_entry_point() const
{
    return desc.entrypoint;
}

std::span<const uint32_t> Shader::get_bytecode() const
{
    return desc.bytecode;
}

const ShaderDesc& Shader::get_desc() const
{
    return desc;
}

ResourceFormatID CompositeShader::get_format_id() const
{
    return "cshd";
}

Ref<Shader> CompositeShader::get_shader( ShaderType stage )
{
    return shaders[stage];
}

void CompositeShader::set_shader( ShaderType stage, const Ref<Shader>& shader )
{
    shaders[stage] = shader;
}

} // namespace nc
