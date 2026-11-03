// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <ncore/runtime/resources/shader.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/servicedb.h>
#include <ncore/services/video/render_service.h>

#include "service_helpers.h"

namespace nc {

Shader::Shader( const ShaderModuleDesc& p_desc )
{
    stages.push_back( p_desc );
    build_configs();
}

Shader::Shader( DynamicArray<ShaderModuleDesc> p_stages_desc ) : stages( std::move( p_stages_desc ) )
{
    build_configs();
}

Shader::~Shader()
{
    release();
}

ResourceFormatID Shader::get_format_id() const
{
    return "nshd";
}

void Shader::on_deserialized()
{
    build_configs();
}

RID Shader::prepare( Scene& p_scene )
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        return prepared_rid;

    if (stages.empty())
        return prepared_rid;

    RenderService* renderer_now = p_scene.get_app_ctx()->services.resolve<RenderService>();
    if (!renderer_now)
        return prepared_rid;

    // The old handle belongs to a device that is gone; drop it without touching it.
    release();

    ShaderProgramDesc desc{};
    desc.debug_name  = StringView( filepath );
    desc.modules     = Span<const ShaderModuleDesc>( stages.data(), stages.size() );
    desc.params      = Span<const ShaderParamDesc>( unified_params.data(), unified_params.size() );
    desc.render_type = get_render_type();

    RID rid = renderer_now->shader_program_create( desc );
    if (!rid.is_valid())
        return prepared_rid;

    renderer = renderer_now;
    set_prepared( rid, renderer_now->get_device_revision() );
    return prepared_rid;
}

void Shader::release()
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        renderer->destroy_rid( prepared_rid );
    prepared_rid = RID::INVALID;
    renderer     = nullptr;
}

size_t Shader::get_size_bytes() const
{
    size_t total = sizeof( Shader );
    for (const auto& stage : stages) {
        total += stage.bytecode.size() * sizeof( uint32_t );
        total += stage.params.size() * sizeof( ShaderParamDesc );
    }
    return total;
}

StringView Shader::get_entry_point( gfx::ShaderStage stage ) const
{
    if (stage == gfx::ShaderStage::NONE && !stages.empty()) {
        return stages[0].entrypoint;
    }
    if (auto desc = get_stage_desc( stage )) {
        return desc->entrypoint;
    }
    return {};
}

Span<const uint32_t> Shader::get_bytecode( gfx::ShaderStage stage ) const
{
    if (stage == gfx::ShaderStage::NONE && !stages.empty()) {
        return stages[0].bytecode;
    }
    if (auto desc = get_stage_desc( stage )) {
        return desc->bytecode;
    }
    return {};
}

Span<const ShaderParamDesc> Shader::get_params() const
{
    return unified_params;
}

Span<const ShaderParamDesc> Shader::get_param_set( gfx::BindingSet p_binding_set ) const
{
    auto it = param_set_lookup.find( p_binding_set );
    if (it != param_set_lookup.end()) {
        return it->second;
    }
    return {};
}

const ShaderParamDesc* Shader::find_param( StringView p_name ) const
{
    auto it = param_name_lookup.find( p_name );
    if (it != param_name_lookup.end()) {
        return &unified_params[it->second];
    }
    return nullptr;
}

bool Shader::has_stage( gfx::ShaderStage stage ) const
{
    return std::any_of( stages.begin(), stages.end(), [stage]( const ShaderModuleDesc& s ) {
        return s.stage == stage;
    } );
}

const ShaderModuleDesc* Shader::get_stage_desc( gfx::ShaderStage stage ) const
{
    auto it =
        std::find_if( stages.begin(), stages.end(), [stage]( const ShaderModuleDesc& s ) { return s.stage == stage; } );
    return it != stages.end() ? &*it : nullptr;
}

Span<const ShaderModuleDesc> Shader::get_stages() const
{
    return stages;
}

gfx::ShaderStage Shader::get_stage_flags() const
{
    gfx::ShaderStage flags = gfx::ShaderStage::NONE;
    for (auto& desc : stages) {
        flags = flags | desc.stage;
    }
    return flags;
}

void Shader::build_configs()
{
    // FIXME: this sucks. too complicated.
    // Algorithm:
    // - populate unified_params by adding over all params from all stages, using param_name_lookup hashmap for dedup.
    // - then we need to populate param_set_lookup, which is a span view over unified_params by BindingSet for memory
    // efficiency.
    // - that is not possible without sorting unified_params first, so that twe can have contiguous array ordered by
    // BindingSet.
    // - patch param_name_lookup indexes after sorting.
    // - populate param_set_lookup by constructing spans over a BindingSet subregion of unified_param.

    unified_params.clear();
    param_name_lookup.clear();

    for (const auto& stage_desc : stages) {
        for (const auto& param : stage_desc.params) {
            auto it = param_name_lookup.find( param.qualified_name );
            if (it != param_name_lookup.end()) {
                // parameter already exists: union stage mask
                auto& existing      = unified_params[it->second];
                existing.stage_mask = existing.stage_mask | stage_desc.stage;

                // A stage that does not use a parameter gets no descriptor binding for it
                // (the SPIR-V drops it), and reflection marks it with the unbound set
                // (0xFFFF, see kUnboundBindingSet in shader_compiler.cpp). Keep whichever
                // stage reported a real set, otherwise the merged param lands in set 0xFFFF
                // and never reaches the resource signature that the PSO needs.
                constexpr gfx::BindingSet UNBOUND = 0xFFFFu;
                if (existing.binding_space == UNBOUND && param.binding_space != UNBOUND) {
                    existing.binding_space = param.binding_space;
                    existing.binding_idx   = param.binding_idx;
                }

                // Each stage reflects the block against its own SPIR-V. A stage whose
                // entry point dropped the block (or a stage that runs before the block
                // shows up in SPIR-V) reports the smaller layout, so keep the largest.
                if (param.total_size_bytes > existing.total_size_bytes)
                    existing.total_size_bytes = param.total_size_bytes;
            } else {
                // new parameter insertion
                ShaderParamDesc unified_param = param;
                unified_param.stage_mask      = static_cast<gfx::ShaderStage>( stage_desc.stage );

                size_t index = unified_params.size();
                unified_params.push_back( std::move( unified_param ) );
                param_name_lookup[param.qualified_name] = index;
            }
        }
    }

    std::stable_sort(
        unified_params.begin(), unified_params.end(),
        []( const ShaderParamDesc& a, const ShaderParamDesc& b ) -> bool { return a.binding_space < b.binding_space; }
    );

    // indices captured in param_name_lookup during the insertion pass are now
    // invalid because the sort permuted unified_params, we need to rebuild it
    param_name_lookup.clear();
    for (size_t idx = 0; idx < unified_params.size(); ++idx) {
        param_name_lookup[unified_params[idx].qualified_name] = idx;
    }

    param_set_lookup.clear();
    size_t i = 0;
    while (i < unified_params.size()) {
        auto set           = unified_params[i].binding_space;
        const size_t start = i;
        while (i < unified_params.size() && unified_params[i].binding_space == set) {
            ++i;
        }
        param_set_lookup[set] = Span<const ShaderParamDesc>( unified_params.data() + start, i - start );
    }
}

ResourceFormatID MaterialShader::get_format_id() const
{
    return "nmts";
}

MaterialStateDesc MaterialShader::get_state() const
{
    MaterialStateDesc state;
    state.cull_mode   = cull_mode;
    state.depth_test  = depth_test;
    state.depth_write = depth_write;
    state.blend       = blend;
    state.multisample = multisample_state;
    return state;
}

} // namespace nc
