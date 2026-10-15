#pragma once

#include <cstdint>

#include <ncore/core/reference.h>

namespace nc {
class Mesh;
} // namespace nc

namespace sea {

/**
 * @brief Build a single camera-centered radial (polar) ocean mesh.
 *
 * Dense near the origin, exponentially sparser outward, covering up to
 * `outer_radius`. Y-up, centered on the origin (the node's transform places it
 * under the camera). The center is closed with a small fan at `inner_radius`.
 *
 * Rings are distributed in two stages: a uniform band of `near_spacing`-metre
 * rings out to `near_radius`, then geometric growth by `ring_growth` out to
 * `outer_radius`. The uniform band keeps the near-field tessellation fine enough
 * that world-anchored vertex displacement does not visibly boil as the camera
 * moves; the geometric tail fills the horizon cheaply.
 *
 * @param num_segments Angular segments.
 * @param num_rings    Max radial rings (may stop earlier at outer_radius).
 * @param inner_radius Radius of the innermost ring (keep > 0 to avoid a degenerate pole).
 * @param outer_radius Hard cap on the outermost radius.
 * @param ring_growth  Geometric growth factor between consecutive rings.
 * @param near_radius  Radius up to which rings are spaced uniformly.
 * @param near_spacing Metre spacing between rings inside `near_radius`.
 */
nc::Ref<nc::Mesh> create_polar_plane_mesh(
    uint32_t num_segments, uint32_t num_rings, float inner_radius, float outer_radius, float ring_growth,
    float near_radius, float near_spacing
);

} // namespace sea
