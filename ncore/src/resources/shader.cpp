#include <algorithm>

#include <ncore/resources/shader.h>

namespace nc {

Shader::Shader( const ShaderDesc& p_desc )
{
    stages.push_back( p_desc );
    build_configs_();
}

Shader::Shader( DynamicArray<ShaderDesc> p_stages_desc ) : stages( p_stages_desc )
{
    build_configs_();
}

ResourceFormatID Shader::get_format_id() const
{
    return "shad";
}

size_t Shader::get_size_bytes() const
{
    size_t total = sizeof( Shader );
    for (const auto& stage : stages) {
        total += stage.bytecode.size() * sizeof( uint32_t );
        total += stage.params.size() * sizeof( ShaderParamInfo );
    }
    return total;
}

StringView Shader::get_entry_point( rhi::ShaderStage stage ) const
{
    if (stage == rhi::ShaderStage::NONE && !stages.empty()) {
        return stages[0].entrypoint;
    }
    if (auto desc = get_stage_desc( stage )) {
        return desc->entrypoint;
    }
    return {};
}

Span<const uint32_t> Shader::get_bytecode( rhi::ShaderStage stage ) const
{
    if (stage == rhi::ShaderStage::NONE && !stages.empty()) {
        return stages[0].bytecode;
    }
    if (auto desc = get_stage_desc( stage )) {
        return desc->bytecode;
    }
    return {};
}

Span<const ShaderParamInfo> Shader::get_params() const
{
    return unified_params;
}

const ShaderParamInfo* Shader::find_param( String name ) const
{
    auto it = param_lookup.find( String( name ) );
    if (it != param_lookup.end()) {
        return &unified_params[it->second];
    }
    return nullptr;
}

bool Shader::has_stage( rhi::ShaderStage stage ) const
{
    return std::any_of( stages.begin(), stages.end(), [stage]( const ShaderDesc& s ) { return s.stage == stage; } );
}

const ShaderDesc* Shader::get_stage_desc( rhi::ShaderStage stage ) const
{
    auto it = std::find_if( stages.begin(), stages.end(), [stage]( const ShaderDesc& s ) { return s.stage == stage; } );
    return it != stages.end() ? it._Ptr : nullptr;
}

Span<const ShaderDesc> Shader::get_stages() const
{
    return stages;
}

rhi::ShaderStage Shader::get_stage_flags() const
{
    rhi::ShaderStage flags = rhi::ShaderStage::NONE;
    for (auto& desc : stages) {
        flags = flags | desc.stage;
    }
    return flags;
}

void Shader::build_configs_()
{
    unified_params.clear();
    param_lookup.clear();

    for (const auto& stage_desc : stages) {
        for (const auto& param : stage_desc.params) {
            auto it = param_lookup.find( param.name );
            if (it != param_lookup.end()) {
                // parameter already exists: union stage mask
                unified_params[it->second].stage_mask = unified_params[it->second].stage_mask | stage_desc.stage;
            } else {
                // new parameter insertion
                ShaderParamInfo unified_param = param;
                unified_param.stage_mask      = static_cast<rhi::ShaderStage>( stage_desc.stage );

                size_t index = unified_params.size();
                unified_params.push_back( std::move( unified_param ) );
                param_lookup[param.name] = index;
            }
        }
    }
}

} // namespace nc
