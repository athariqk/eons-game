#pragma once

#include <ncore.h>
#include <ncore/core/collection.h>
#include <ncore/core/reference.h>

namespace nc {

class Image;

/**
 * @brief Convert an equirectangular image into 6 cube face images.
 *
 * Assumes the source image's top row corresponds to zenith (+Y) and the
 * horizontal center (u = 0.5) corresponds to the +X direction.
 *
 * Face order: +X, -X, +Y, -Y, +Z, -Z (Vulkan/Diligent cubemap order).
 * Output faces are RGBA8, bilinearly sampled from the source.
 *
 * @param equirect  Source equirectangular image (2:1 aspect recommended).
 * @param face_size Output face resolution (width == height).
 * @return 6 cube face images.
 */
NCAPI Array<Ref<Image>, 6> equirect_to_cube( const Image& equirect, uint32_t face_size );

} // namespace nc
