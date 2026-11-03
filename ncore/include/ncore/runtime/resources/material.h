// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: material.h

#pragma once

#include <ncore/types/variant.h>

#include "shader.h"

namespace nc {

/**
 * @brief One reflected parameter of a material's binding set.
 *
 * A material parameter is either a uniform (a value inside the material's
 * constant buffer) or a texture. Which one it is comes from the shader
 * reflection, never from the caller: Material::set_param() looks the name up
 * here and rejects a value for a texture slot and a resource for a uniform slot.
 */
struct MaterialParam {
    enum class Kind : uint8_t {
        UNIFORM, // a value inside the g_Material constant buffer
        TEXTURE, // a texture (or texture array/cube) binding
    };

    Kind kind = Kind::UNIFORM;
    /**
     * @brief Reflected parameter name (base name only for nested structs).
     */
    String name;

    // Editor presentation, copied from the shader reflection.
    String header;
    gfx::ParamWidget widget = gfx::ParamWidget::DEFAULT;
    float range_min         = 0.0f;
    float range_max         = 0.0f;

    // UNIFORM
    uint32_t offset                     = 0; // byte offset inside the material's parameter block
    uint32_t size                       = 0; // byte size of the whole field (all array elements)
    uint32_t stride                     = 0; // GPU element stride (std140 may pad it out)
    uint32_t array_size                 = 1;
    gfx::ParameterType type             = gfx::ParameterType::UNKNOWN;
    const types::TypeInfo* element_type = nullptr;

    // TEXTURE
    gfx::ResourceType resource_type = gfx::ResourceType::CONSTANT_BUFFER;

    // Staged value. For UNIFORM parameters this is the value or a tightly-packed
    // array of values; for TEXTURE parameters it is the staged RID.
    TypedVar value;
    RID texture_rid       = RID::INVALID; // TEXTURE: RID staged by set_param()
    RID bound_texture_rid = RID::INVALID; // last RID handed to the renderer

    bool is_texture() const noexcept
    {
        return kind == Kind::TEXTURE;
    }

    /**
     * @brief Number of array elements currently staged (1 for a scalar).
     */
    uint32_t value_count() const noexcept
    {
        return kind == Kind::UNIFORM && value.has_value() ? static_cast<uint32_t>( value.count() ) : 0;
    }
};

/**
 * @brief Material is a MaterialShader instance with bound textures and uniform values.
 */
class NCAPI Material : public IResource {
    NCLASS( Material, IResource )

public:
    Material() = default;
    ~Material() override;

    ResourceFormatID get_format_id() const override;

    RID prepare( Scene& p_scene ) override;
    void on_deserialized() override;

    void set_shader( const Ref<MaterialShader>& p_shader );
    Ref<MaterialShader> get_shader() const;

    /**
     * @brief Set a reflected material parameter (upsert).
     *
     * @param p_name  Reflected parameter name.
     * @param p_value Value or texture RID, boxed by the caller.
     */
    void set_param( StringView p_name, const TypedVar& p_value );

    /**
     * @brief Resolve a reflected parameter by name.
     *
     * @return The staged value of the parameter, or an invalid TypedVar when the shader
     *         does not declare the parameter. Invalidated by set_shader().
     */
    TypedVar get_param( StringView p_name ) const;

    Span<const MaterialParam> get_params() const NC_LIFETIME_BOUND
    {
        return param_layout;
    }

    void set_draw_mode( gfx::FillMode p_mode );
    gfx::FillMode get_draw_mode() const;

private:
    static constexpr size_t NOT_FOUND = static_cast<size_t>( -1 );

    size_t index_of( StringView p_name ) const;

    void rebuild_layout();
    void release() override;

    REFLECT Ref<MaterialShader> shader;
    REFLECT gfx::FillMode draw_mode = gfx::FillMode::SOLID;

    DynamicArray<MaterialParam> param_layout;
    uint32_t param_buffer_size = 0;
    bool param_dirty           = false;
    bool texture_dirty         = false;

    RenderService* renderer  = nullptr;
    uint32_t shader_revision = 0;
    uint32_t layout_revision = 0;
};

} // namespace nc
