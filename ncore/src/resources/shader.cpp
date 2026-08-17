#include <ncore/resources/shader.h>

namespace nc {

Shader::Shader( const ShaderDesc& p_desc ) : desc( p_desc ) {}

ResourceFormatID Shader::get_format_id() const
{
    return "shad";
}

size_t Shader::get_size_bytes() const
{
    return desc.bytecode.size() * sizeof( uint32_t );
}

ShaderType Shader::get_stage() const
{
    return desc.stage;
}

StringView Shader::get_entry_point() const
{
    return desc.entrypoint;
}

Span<const uint32_t> Shader::get_bytecode() const
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

size_t CompositeShader::get_size_bytes() const
{
    size_t total = 0;
    for (auto& shader : shaders) {
        total += shader.second->get_size_bytes();
	}
    return total;
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
