#pragma once

#include <cstdint>

namespace ncore {

struct ResourceHandle {
    ResourceID id = SIZE_MAX;
    uint32_t generation = 0;

    bool is_valid() const { return id != SIZE_MAX; }
    bool operator==(const ResourceHandle &other) const {
        return id == other.id && generation == other.generation;
    }
    bool operator!=(const ResourceHandle &other) const { return !(*this == other); }
};

} // namespace ncore
