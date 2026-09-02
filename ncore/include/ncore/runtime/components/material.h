#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>
#include <ncore/services/video/rhi_types.h>

namespace nc {

class MaterialTemplate;

constexpr int MAXIMUM_MATERIAL_TEXTURE = 8;

/**
 * Bitmask of MaterialComponent fields that need to be pushed to the GPU material instance.
 * Gameplay/editor code mutates the component and sets Dirty; video plugin observers push.
 */
enum MaterialDirty : uint32_t {
    MATERIAL_DIRTY_NONE      = 0,
    MATERIAL_DIRTY_DRAW_MODE = 1u << 0,
    MATERIAL_DIRTY_TEXTURES  = 1u << 1,
    MATERIAL_DIRTY_ALL       = MATERIAL_DIRTY_DRAW_MODE | MATERIAL_DIRTY_TEXTURES,
};

struct NCAPI MaterialComponent {
    RID Source                                    = 0;
    RID Instance                                  = 0;
    Array<RID, MAXIMUM_MATERIAL_TEXTURE> Textures = {};
    int TextureCount                              = 0;
    rhi::FillMode DrawMode                        = rhi::FillMode::SOLID;

    /**
     * Fields awaiting push to the GPU instance (see MaterialDirty).
     * Cleared by the video plugin after a successful sync.
     */
    uint32_t Dirty = MATERIAL_DIRTY_NONE;

    void mark_dirty( uint32_t flags = MATERIAL_DIRTY_ALL ) { Dirty |= flags; }

    void set_draw_mode( rhi::FillMode mode )
    {
        DrawMode = mode;
        Dirty |= MATERIAL_DIRTY_DRAW_MODE;
    }

    /**
     * @brief Add a GPU texture to this material.
     *
     * Obtain GPU textures from RenderService.texture_x_create() methods.
     * Marks textures dirty so the video plugin rebinds GPU slots on the next OnSet.
     */
    void add_texture( RID texture_rid )
    {
        NC_FAIL_MSG_RET( TextureCount < MAXIMUM_MATERIAL_TEXTURE, "Max texture count reached." );
        Textures[TextureCount++] = texture_rid;
        Dirty |= MATERIAL_DIRTY_TEXTURES;
    }

    void set_texture( int slot, RID texture_rid )
    {
        NC_FAIL_MSG_RET( slot >= 0 && slot < MAXIMUM_MATERIAL_TEXTURE, "Texture slot out of range." );
        Textures[slot] = texture_rid;
        if (slot >= TextureCount)
            TextureCount = slot + 1;
        Dirty |= MATERIAL_DIRTY_TEXTURES;
    }

    NSTRUCTV(
        MaterialComponent, NC_F( MaterialComponent, Source ), NC_F( MaterialComponent, Instance ),
        NC_F( MaterialComponent, Textures ), NC_F( MaterialComponent, TextureCount ),
        NC_F( MaterialComponent, DrawMode ), NC_F( MaterialComponent, Dirty )
    )
};

} // namespace nc
