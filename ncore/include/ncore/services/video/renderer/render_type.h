#pragma once

#include <ncore/types/typedb.h>

namespace nc {

enum class RenderType : uint8_t {
    SPATIAL,
    CANVAS
};
NENUM( RenderType, NENUM_ELEMENT( RenderType, SPATIAL ), NENUM_ELEMENT( RenderType, CANVAS ) )

} // namespace nc
