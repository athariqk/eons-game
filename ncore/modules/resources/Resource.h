#pragma once

#include <cstdint>

#include <modules/utils/Structures.h>

namespace ncore {

/**
 * @brief Base type for all resource data.
 */
struct Resource {
    virtual size_t get_size_in_bytes() const { return 0; }
};

/**
 * @brief Represents an image in RGBA32 pixel format.
 */
struct Image : Resource {
    BytesBuffer data;
    float width = 0;
    float height = 0;
    size_t get_size_in_bytes() const override { return data.size(); }
};

struct AudioClip : Resource {
    BytesBuffer data;
    size_t length = 0;
    int channels;
    int frequency;
    int bits_per_sample;
    size_t get_size_in_bytes() const { return data.size(); }
};

} // namespace ncore
