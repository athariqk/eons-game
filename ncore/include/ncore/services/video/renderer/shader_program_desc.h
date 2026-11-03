// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/core/collection.h>
#include <ncore/services/video/gfx_types.h>
#include <ncore/types/typedb.h>

#include "render_type.h"

namespace nc {

/**
 * @brief One reflected field of a shader uniform block.
 */
struct REFLECT NCAPI ShaderParamField {
    REFLECT String qualified_name; // e.g. "g_Material.diffuse_color"
    REFLECT String base_name;      // e.g. "diffuse_color"
    REFLECT uint32_t offset;
    REFLECT uint32_t size;
    REFLECT size_t stride;
    REFLECT gfx::ParameterType type;
    REFLECT uint32_t array_size;
    // Editor presentation, from Slang custom attributes on the uniform.
    REFLECT gfx::ParamWidget widget = gfx::ParamWidget::DEFAULT;
    REFLECT float range_min         = 0.0f;
    REFLECT float range_max         = 0.0f;
    REFLECT String header;
};

/**
 * @brief A reflected shader parameter (uniform block, texture, sampler, varying...).
 */
struct REFLECT NCAPI ShaderParamDesc {
    REFLECT String qualified_name;
    REFLECT String base_name;
    REFLECT String semantic_name;
    REFLECT gfx::ResourceType resource_type        = gfx::ResourceType::CONSTANT_BUFFER;
    REFLECT gfx::ParameterType param_type          = gfx::ParameterType::UNKNOWN;
    REFLECT gfx::ResourceFlags flags               = gfx::ResourceFlags::NONE;
    REFLECT gfx::ResourceCpuAccessFlags cpu_access = gfx::ResourceCpuAccessFlags::READ;
    REFLECT gfx::BindingSet binding_space          = 0; // or "set" in Vulkan terms.
    REFLECT uint32_t binding_idx                   = 0; // the "binding/register" on the binding_space.
    REFLECT uint32_t location                      = 0; // relevant only for the varying_input resource type.
    REFLECT size_t offset                          = 0;
    REFLECT size_t element_stride                  = 0;
    REFLECT size_t total_size_bytes                = 0;
    REFLECT DynamicArray<ShaderParamField> fields;
    REFLECT gfx::ShaderStage stage_mask = gfx::ShaderStage::NONE; // bitmask of all stages using this param.
};

using ShaderParamLayout = DynamicArray<ShaderParamDesc>;

/**
 * @brief One compiled shader stage of a program.
 */
struct REFLECT NCAPI ShaderModuleDesc {
    REFLECT String name;
    REFLECT gfx::ShaderStage stage;
    REFLECT String entrypoint;
    REFLECT DynamicArray<uint32_t> bytecode;
    REFLECT ShaderParamLayout params;
    REFLECT gfx::VertexLayout vert_layout;
    REFLECT uint32_t num_threads_x = 1;
    REFLECT uint32_t num_threads_y = 1;
    REFLECT uint32_t num_threads_z = 1;
};

/**
 * @brief Description of a GPU shader program which has one or more stages.
 */
struct NCAPI ShaderProgramDesc {
    StringView debug_name;
    Span<const ShaderModuleDesc> modules;
    /// Already unified across stages (one entry per parameter name/set).
    Span<const ShaderParamDesc> params;
    RenderType render_type = RenderType::SPATIAL;
};

} // namespace nc
