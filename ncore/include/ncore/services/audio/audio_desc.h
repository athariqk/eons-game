// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/core/collection.h>
#include <ncore/types/typedb.h>

namespace nc {

/**
 * @brief Neutral description of a PCM audio stream.
 *
 * `data` is borrowed for the duration of the create call only; the service keeps
 * whatever it needs afterwards.
 */
struct NCAPI AudioStreamDesc {
    Span<const std::byte> data;
    int channels        = 0;
    int frequency       = 0;
    int bits_per_sample = 0;
};

} // namespace nc
