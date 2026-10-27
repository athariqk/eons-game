#pragma once

#include <cstdint>

#include <ncore/core/reference.h>

namespace nc {
class Mesh;
} // namespace nc

namespace sea {

struct WaterMeshDesc {
    uint32_t cells       = 64;        // quads per side per level (rounded up to a multiple of 4, min 4)
    uint32_t levels      = 0;         // 0 = auto: enough levels to reach `horizon`
    float base_spacing   = 3.5f;      // quad size of level 0
    float horizon        = 450000.0f; // half extent to reach when levels == 0
    uint32_t chunk_cells = 0;         // create_water_clipmap: quads per chunk side per level (0 = one chunk per level)
};

/**
 * @brief Creates water clipmap mesh.
 */
nc::Ref<nc::Mesh> create_water_clipmap( WaterMeshDesc const& desc = {} );

} // namespace sea
