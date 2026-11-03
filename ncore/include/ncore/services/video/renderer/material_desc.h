// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/services/video/gfx_types.h>
#include <ncore/structs/rid.h>
#include <ncore/types/typedb.h>

#include "render_type.h"

namespace nc {

/**
 * @brief Fixed-function pipeline state a material permutation is baked from.
 */
struct REFLECT NCAPI MaterialStateDesc {
    REFLECT gfx::CullMode cull_mode               = gfx::CullMode::NONE;
    REFLECT bool depth_test                       = false;
    REFLECT bool depth_write                      = false;
    REFLECT gfx::BlendPreset blend                = gfx::BlendPreset::ALPHA_BLEND;
    REFLECT gfx::MultisampleStateDesc multisample = { 1, 0 };
    REFLECT gfx::FillMode fill_mode               = gfx::FillMode::SOLID;
};

/**
 * @brief Description of a GPU material instance creation.
 */
struct NCAPI MaterialCreateDesc {
    RenderType render_type = RenderType::SPATIAL;
    /**
     * @brief A GPU shader program handle produced by RenderService::shader_program_create().
     */
    RID shader = RID::INVALID;
    /**
     * @brief The size of the material binding-set block, already 256-byte aligned by the caller.
     */
    uint32_t param_buffer_size = 0;
    MaterialStateDesc state;
};

} // namespace nc
