// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include "../io/resource.h"

namespace nc {

class NCAPI AudioClip : public IResource {
    NCLASS( AudioClip, IResource )

public:
    AudioClip() = default;
    AudioClip( const void* p_data, int p_length, int p_channels, int p_frequency, int p_bits_per_sample );

    ResourceFormatID get_format_id() const override;
    Span<std::byte> get_data() NC_LIFETIME_BOUND;
    Span<const std::byte> get_data() const NC_LIFETIME_BOUND;

    int get_length() const;
    int get_channels() const;
    int get_frequency() const;
    int get_bits_per_sample() const;

    size_t get_size_bytes() const override;

private:
    REFLECT int length          = 0;
    REFLECT int channels        = 0;
    REFLECT int frequency       = 0;
    REFLECT int bits_per_sample = 0;
    REFLECT BytesBuffer data;
};

} // namespace nc
