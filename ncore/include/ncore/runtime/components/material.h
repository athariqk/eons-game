#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>

namespace nc {

class MaterialTemplate;

constexpr int MAXIMUM_MATERIAL_TEXTURE = 8;

struct NCAPI MaterialComponent {
    RID Source                                    = 0;
    RID Instance                                  = 0;
    Array<RID, MAXIMUM_MATERIAL_TEXTURE> Textures = {};
    int TextureCount                              = 0;

    /**
     * @brief Add a GPU texture to this material.
     *
     * Obtain GPU textures from RenderService.texture_x_create() methods.
     */
    void add_texture( RID texture_rid )
    {
        NC_FAIL_MSG_RET( TextureCount < MAXIMUM_MATERIAL_TEXTURE, "Max texture count reached." );
        Textures[TextureCount++] = texture_rid;
    }

    NSTRUCTV(
        MaterialComponent, NC_F( MaterialComponent, Source ) NC_F( MaterialComponent, Instance )
                               NC_F( MaterialComponent, Textures ) NC_F( MaterialComponent, TextureCount )
    )
};

} // namespace nc
