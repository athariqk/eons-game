// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: material.cpp

#include <algorithm>

#include <ncore/runtime/resources/material.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/servicedb.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/renderer/render_storage.h>
#include <ncore/utils/log.h>

#include "service_helpers.h"

namespace nc {

namespace {

/**
 * @brief Resolve a texture parameter's staged handle.
 *
 * Accepts either a boxed RID or a Ref<T> handle (TypedVar boxes the Ref<T>
 * itself, keeping the pointee alive). A Ref<T> contributes the resource's
 * prepared RID, so the caller must have prepared it first.
 */
RID resolve_texture_rid( const TypedVar& p_value )
{
    if (p_value.is<RID>())
        return p_value.get<RID>();

    if (p_value.is_ref()) {
        ConstTypedPtr pointee = p_value.deref();
        if (auto* res = pointee.try_as<IResource>())
            return res->get_rid();
    }

    return RID();
}

/**
 * @brief Constant-buffer blocks must start on a 256-byte boundary.
 */
constexpr uint32_t PARAM_BUFFER_ALIGNMENT = 256;

/**
 * @brief Map a reflected shader uniform type onto a registered NCORE type.
 *
 * Returns null for types we have no reflection counterpart for (packed formats,
 * half floats, integer vectors beyond Vec2i, ...). Such a parameter is skipped:
 * the shader still reads zeroes from the parameter block, the editor just cannot
 * inspect it.
 */
const types::TypeInfo* type_of( gfx::ParameterType p_type )
{
    switch (p_type) {
        case gfx::ParameterType::BOOL:
            return TypeDB::find<bool>();
        case gfx::ParameterType::INT_8:
            return TypeDB::find<char>();
        case gfx::ParameterType::INT_16:
            return TypeDB::find<short>();
        case gfx::ParameterType::INT_32:
            return TypeDB::find<int>();
        case gfx::ParameterType::INT_64:
            return TypeDB::find<long long>();
        case gfx::ParameterType::UINT:
            return TypeDB::find<unsigned int>();
        case gfx::ParameterType::USHORT:
            return TypeDB::find<unsigned short>();
        case gfx::ParameterType::UBYTE:
            return TypeDB::find<unsigned char>();
        case gfx::ParameterType::ULONG:
            return TypeDB::find<unsigned long long>();
        case gfx::ParameterType::FLOAT_32:
            return TypeDB::find<float>();
        case gfx::ParameterType::FLOAT_64:
            return TypeDB::find<double>();
        case gfx::ParameterType::FLOAT2:
            return TypeDB::find<Vec2f>();
        case gfx::ParameterType::FLOAT3:
            return TypeDB::find<Vec3f>();
        case gfx::ParameterType::FLOAT4:
            return TypeDB::find<Vec4f>();
        case gfx::ParameterType::INT2:
            return TypeDB::find<Vec2i>();
        case gfx::ParameterType::MAT3:
            return TypeDB::find<Mat3>();
        case gfx::ParameterType::MAT4:
            return TypeDB::find<Mat4>();
        default:
            return nullptr;
    }
}

/**
 * @brief Whether a reflected binding-space-1 parameter is a texture slot.
 */
bool is_texture_param( gfx::ResourceType p_type )
{
    return p_type == gfx::ResourceType::TEXTURE_SRV || p_type == gfx::ResourceType::TEXTURE_UAV;
}

} // namespace

Material::~Material()
{
    release();
}

ResourceFormatID Material::get_format_id() const
{
    return "nmat";
}

RID Material::prepare( Scene& p_scene )
{
    if (!shader)
        return prepared_rid;

    // A hot-reloaded or newly deserialized shader changes the reflected layout
    // without going through set_shader().
    if (layout_revision != shader->get_revision())
        rebuild_layout();

    const RID shader_rid = shader->prepare( p_scene );
    if (!shader_rid)
        return prepared_rid;

    RenderService* renderer_now = p_scene.get_app_ctx()->services.resolve<RenderService>();
    if (!renderer_now)
        return prepared_rid;

    const bool needs_rebuild = !is_prepared_alive( renderer, prepared_rid, service_revision ) ||
                               renderer != renderer_now || shader->get_revision() != shader_revision;

    if (needs_rebuild) {
        release();

        MaterialCreateDesc desc{};
        desc.render_type       = shader->material_type;
        desc.shader            = shader_rid;
        desc.param_buffer_size = param_buffer_size;
        desc.state             = shader->get_state();
        desc.state.fill_mode   = draw_mode;

        prepared_rid = renderer_now->material_create( desc );
        if (!prepared_rid)
            return prepared_rid;

        renderer        = renderer_now;
        shader_revision = shader->get_revision();
        param_dirty     = true;
        texture_dirty   = true;
        set_prepared( prepared_rid, renderer_now->get_device_revision() );

        NC_LOG_DEBUG_C(
            log::GRAPHICS, "Material '{}' prepared: rid={} shader_rid={} param_buffer={} cause={}", shader->filepath,
            prepared_rid.value, shader_rid.value, param_buffer_size, "rebuild"
        );
    }

    if (!param_dirty && !texture_dirty)
        return prepared_rid;

    for (auto& param : param_layout) {
        if (param.kind == MaterialParam::Kind::UNIFORM) {
            if (!param.value.has_value() || !param.element_type)
                continue;

            const uint32_t count = static_cast<uint32_t>( std::min<size_t>( param.value.count(), param.array_size ) );
            const auto element_size = static_cast<uint32_t>( param.element_type->size );
            if (count <= 1) {
                renderer_now->material_set_data(
                    prepared_rid, param.offset, param.value.view().get_ptr(), element_size
                );
                continue;
            }

            // Write element-by-element: the GPU stride may be padded out (std140),
            // so the packed CPU array cannot be uploaded in one memcpy.
            for (uint32_t i = 0; i < count; ++i) {
                renderer_now->material_set_data(
                    prepared_rid, param.offset + i * param.stride, param.value.element_ptr( i ), element_size
                );
            }
            continue;
        }

        if (!param.texture_rid || param.texture_rid == param.bound_texture_rid)
            continue; // the renderer keeps its fallback texture bound

        renderer_now->material_set_texture( prepared_rid, param.name, param.texture_rid );
        param.bound_texture_rid = param.texture_rid;
    }

    param_dirty   = false;
    texture_dirty = false;
    return prepared_rid;
}

void Material::on_deserialized()
{
    rebuild_layout();
}

void Material::set_shader( const Ref<MaterialShader>& p_shader )
{
    if (shader.get() == p_shader.get())
        return;

    release();
    shader = p_shader;
    rebuild_layout();
}

Ref<MaterialShader> Material::get_shader() const
{
    return shader;
}

static String list_param_names( const DynamicArray<MaterialParam>& p_layout )
{
    String out;
    for (const auto& p : p_layout) {
        if (!out.empty())
            out += ", ";
        out += p.name.empty() ? String( "<EMPTY>" ) : p.name;
        out += p.is_texture() ? "(tex)" : "";
    }
    return out.empty() ? String( "<none>" ) : out;
}

void Material::set_param( StringView p_name, const TypedVar& p_value )
{
    const size_t index = index_of( p_name );
    if (index == NOT_FOUND) {
        NC_LOG_WARN_C(
            log::GRAPHICS, "Shader '{}' does not declare a parameter named '{}'. Known: [{}]",
            shader ? shader->filepath : String( "<no shader>" ), p_name, list_param_names( param_layout )
        );
        return;
    }

    MaterialParam& param = param_layout[index];
    if (param.is_texture()) {
        const RID texture = resolve_texture_rid( p_value );
        if (!texture.is_valid()) {
            NC_LOG_WARN_C( log::GRAPHICS, "Shader texture parameter '{}' expects a RID or a Ref<IResource>", p_name );
            return;
        }
        param.texture_rid = texture;
        texture_dirty     = true;
        return;
    }

    if (!p_value.has_value() || !param.element_type) {
        NC_LOG_WARN_C( log::GRAPHICS, "Shader parameter '{}' got an empty value", p_name );
        return;
    }

    const size_t count = std::min<size_t>( p_value.count(), param.array_size );
    if (param.value.count() != count)
        param.value = TypedVar::array_of( param.element_type, count );

    if (!coerce_value( param.value.view(), count, p_value.view(), count )) {
        NC_LOG_WARN_C(
            log::GRAPHICS, "Shader parameter '{}' expects '{}', got '{}'", p_name, param.element_type->name,
            p_value.get_type()->name
        );
        return;
    }

    param_dirty = true;
}

TypedVar Material::get_param( StringView p_name ) const
{
    const size_t index = index_of( p_name );
    return index == NOT_FOUND ? TypedVar() : param_layout[index].value;
}

void Material::set_draw_mode( gfx::FillMode p_mode )
{
    if (draw_mode == p_mode)
        return;

    draw_mode = p_mode;
    if (prepared_rid && renderer)
        renderer->material_set_draw_mode( prepared_rid, draw_mode );
}

gfx::FillMode Material::get_draw_mode() const
{
    return draw_mode;
}

size_t Material::index_of( StringView p_name ) const
{
    for (size_t i = 0; i < param_layout.size(); ++i) {
        if (StringView( param_layout[i].name ) == p_name)
            return i;
    }
    return NOT_FOUND;
}

void Material::rebuild_layout()
{
    // Values staged on the old layout survive a shader swap / hot reload as long
    // as the new shader still declares a compatible parameter under that name.
    HashMap<String, TypedVar> old_values;
    HashMap<String, RID> old_texture_rids;
    for (auto& param : param_layout) {
        if (param.is_texture()) {
            if (param.texture_rid)
                old_texture_rids[param.name] = param.texture_rid;
        } else if (param.value.has_value()) {
            old_values[param.name] = std::move( param.value );
        }
    }

    param_layout.clear();
    param_buffer_size = 0;
    param_dirty       = false;
    texture_dirty     = false;
    layout_revision   = shader ? shader->get_revision() : 0;

    if (!shader)
        return;

    uint32_t block_size = 0;
    uint32_t field_end  = 0;

    for (const auto& desc : shader->get_param_set( RenderStorage::BINDING_SET_MATERIAL )) {
        if (desc.resource_type == gfx::ResourceType::CONSTANT_BUFFER) {
            block_size = std::max( block_size, static_cast<uint32_t>( desc.total_size_bytes ) );

            for (const auto& field : desc.fields) {
                const types::TypeInfo* element_type = type_of( field.type );
                if (!element_type) {
                    NC_LOG_WARN_C(
                        log::GRAPHICS, "Material '{}': parameter '{}' has no reflected counterpart", shader->filepath,
                        field.base_name
                    );
                    continue;
                }

                auto& param        = param_layout.emplace_back();
                param.kind         = MaterialParam::Kind::UNIFORM;
                param.name         = field.base_name;
                param.header       = field.header;
                param.widget       = field.widget;
                param.range_min    = field.range_min;
                param.range_max    = field.range_max;
                param.offset       = field.offset;
                param.size         = field.size;
                param.stride       = field.stride > 0 ? static_cast<uint32_t>( field.stride )
                                                      : static_cast<uint32_t>( element_type->size );
                param.array_size   = field.array_size > 0 ? field.array_size : 1;
                param.type         = field.type;
                param.element_type = element_type;
                param.value        = TypedVar::default_of( element_type );

                if (auto it = old_values.find( param.name ); it != old_values.end()) {
                    const TypedVar& previous = it->second;
                    if (previous.get_type() == param.element_type && previous.count() > 0) {
                        const size_t count = std::min<size_t>( previous.count(), param.array_size );
                        if (param.value.count() != count)
                            param.value = TypedVar::array_of( param.element_type, count );
                        coerce_value( param.value.view(), count, previous.view(), count );
                    }
                }

                // std140 may pad the last element; take the larger of the declared
                // field size and the strided span so the block can never overrun.
                const uint32_t span = param.array_size * param.stride;
                field_end           = std::max( field_end, param.offset + std::max( param.size, span ) );
            }
            continue;
        }

        // Samplers and the rest of the binding are renderer-owned: there is no
        // user-settable value behind them.
        if (!is_texture_param( desc.resource_type ))
            continue;

        auto& param         = param_layout.emplace_back();
        param.kind          = MaterialParam::Kind::TEXTURE;
        param.name          = desc.base_name;
        param.resource_type = desc.resource_type;
        if (auto rid = old_texture_rids.find( param.name ); rid != old_texture_rids.end())
            param.texture_rid = rid->second;
    }

    // The block's own reflected size is authoritative; the per-field extent is the
    // fallback for shaders whose block size reflection does not report.
    const uint32_t wanted = std::max( block_size, field_end );
    if (wanted > 0)
        param_buffer_size = ( wanted + PARAM_BUFFER_ALIGNMENT - 1 ) & ~( PARAM_BUFFER_ALIGNMENT - 1 );

    param_dirty   = param_buffer_size > 0;
    texture_dirty = true;
}

void Material::release()
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        renderer->destroy_rid( prepared_rid );

    prepared_rid    = RID::INVALID;
    renderer        = nullptr;
    shader_revision = 0;
    param_dirty     = param_buffer_size > 0;
    texture_dirty   = true;

    for (auto& param : param_layout)
        param.bound_texture_rid = RID::INVALID;
}

} // namespace nc
