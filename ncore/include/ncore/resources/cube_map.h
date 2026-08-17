#pragma once

#include "image.h"
#include "resource.h"

namespace nc {

/**
 * @brief CubeMap represents cube-mapped image.
 */
class NCAPI CubeMap : public IResource {
    NCLASS( CubeMap, IResource )

public:
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
     */
    CubeMap( const Ref<Image>& equirect, uint32_t face_size = 0 );

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    Span<const Ref<Image>, 6> get_faces() const;

private:
    Array<Ref<Image>, 6> faces;
};

} // namespace nc
