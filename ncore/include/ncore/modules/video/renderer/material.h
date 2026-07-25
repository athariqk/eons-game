#pragma once

#include <string>

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>

namespace nc {

struct Material {
    RID pso;
    Vector<RID> resource_signatures;
    Vector<RID> srbs;
    RID sampler;
    RID constant_buffer;

    struct TextureSlot {
        std::string name;
        size_t srb_index;
    };

    Vector<TextureSlot> texture_slots;
};

} // namespace nc
