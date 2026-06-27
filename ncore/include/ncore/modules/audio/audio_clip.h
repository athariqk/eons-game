#pragma once

#include <ncore/kernel/structures.h>
#include <ncore/modules/assets/asset.h>

namespace ncore {

struct AudioClip : IAssetResource {
    BytesBuffer data;
    size_t length = 0;
    int channels;
    int frequency;
    int bits_per_sample;
    size_t get_size_in_bytes() const override
    {
        return data.size();
    }
};

} // namespace ncore
