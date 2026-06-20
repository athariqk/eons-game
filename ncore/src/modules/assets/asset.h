#pragma once

#include <cstdint>

namespace ncore {

struct IAssetResource {
    virtual size_t get_size_in_bytes() const { return 0; }
};

} // namespace ncore
