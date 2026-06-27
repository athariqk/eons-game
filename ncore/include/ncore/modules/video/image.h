#pragma once

#include <ncore/kernel/structures.h>
#include <ncore/modules/assets/asset.h>

namespace ncore {

/**
 * @brief Represents an image in RGBA32 pixel format.
 */
struct Image : IAssetResource {
    BytesBuffer data;
    float width  = 0;
    float height = 0;
    size_t get_size_in_bytes() const override
    {
        return data.size();
    }
};

} // namespace ncore
