// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/services/video/renderer/material_desc.h>
#include <ncore/services/video/renderer/shader_program_desc.h>

#include "../io/resource.h"

namespace nc {

class RenderService;

/**
 * @brief Shader resource represents compiled GPU program(s)
 * or a multi-stage pipeline.
 */
class NCAPI Shader : public IResource {
    NCLASS( Shader, IResource )

public:
    Shader() = default;
    /**
     * @brief Construct single-stage shader.
     */
    Shader( const ShaderModuleDesc& p_desc );
    /**
     * @brief Construct multi-stage shader.
     */
    Shader( DynamicArray<ShaderModuleDesc> p_stages_desc );
    ~Shader() override;

    ResourceFormatID get_format_id() const override;

    size_t get_size_bytes() const override;

    RID prepare( Scene& p_scene ) override;

    void on_deserialized() override;

    /**
     * @brief Get the shader's entrypoint name.
     */
    StringView get_entry_point( gfx::ShaderStage stage ) const NC_LIFETIME_BOUND;

    /**
     * @brief Get the shader's compiled bytecode.
     */
    Span<const uint32_t> get_bytecode( gfx::ShaderStage stage ) const NC_LIFETIME_BOUND;

    /**
     * @brief Return shader parameter descriptions from all available stages.
     * @return Array view of the reflected parameter descriptions.
     */
    Span<const ShaderParamDesc> get_params() const NC_LIFETIME_BOUND;
    /**
     * @brief Get specific set of shader parameter descriptions from all available stages.
     * @param p_binding_set The shader's binding set.
     * @return Array view of the matching reflected parameter descriptions.
     */
    Span<const ShaderParamDesc> get_param_set( gfx::BindingSet p_binding_set ) const;
    /**
     * @brief Find a shader parameter info by name.
     */
    const ShaderParamDesc* find_param( StringView p_name ) const NC_LIFETIME_BOUND;

    bool has_stage( gfx::ShaderStage stage ) const;
    const ShaderModuleDesc* get_stage_desc( gfx::ShaderStage stage ) const NC_LIFETIME_BOUND;

    /**
     * @brief Get each stage attributes making up this shader.
     */
    Span<const ShaderModuleDesc> get_stages() const NC_LIFETIME_BOUND;

    /**
     * @brief Return the shader stages of this shader as a mask.
     */
    gfx::ShaderStage get_stage_flags() const;

    /**
     * @brief Which render pipeline family this shader is drawn with.
     */
    virtual RenderType get_render_type() const
    {
        return RenderType::SPATIAL;
    }

protected:
    void release() override;

    void build_configs();

    REFLECT DynamicArray<ShaderModuleDesc> stages;
    ShaderParamLayout unified_params;
    HashMap<gfx::BindingSet, Span<const ShaderParamDesc>> param_set_lookup;
    HashMap<StringView, size_t> param_name_lookup;

    RenderService* renderer = nullptr;
};

// --------------------------------------------------------------------------------------

/**
 * @brief MaterialShader is a shader that has been given a specific contract
 * with NCORE's material system.
 *
 * Loosely inspired by EsotericaEngine's MaterialShader concept.
 */
class NCAPI MaterialShader : public Shader {
    NCLASS( MaterialShader, Shader )

public:
    MaterialShader() = default;
    MaterialShader( DynamicArray<ShaderModuleDesc> p_stages_desc ) : Shader( std::move( p_stages_desc ) ) {}
    ~MaterialShader() override = default;

    ResourceFormatID get_format_id() const override;

    /**
     * @brief The fixed-function pipeline state this material shader declares.
     */
    MaterialStateDesc get_state() const;

    RenderType get_render_type() const override
    {
        return material_type;
    }

    REFLECT RenderType material_type                    = RenderType::SPATIAL;
    REFLECT gfx::CullMode cull_mode                     = gfx::CullMode::NONE;
    REFLECT bool depth_test                             = false;
    REFLECT bool depth_write                            = false;
    REFLECT gfx::BlendPreset blend                      = gfx::BlendPreset::ALPHA_BLEND;
    REFLECT gfx::MultisampleStateDesc multisample_state = { 1, 0 };
};

} // namespace nc
