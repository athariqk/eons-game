// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include "../io/resource.h"

namespace nc {

class NCAPI Font : public IResource {
    NCLASS( Font, IResource )

public:
    Font() = default;

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    Span<const std::byte> get_data() const NC_LIFETIME_BOUND;

    void set_data( const void* p_data, size_t p_size );

private:
    REFLECT BytesBuffer data;
};

} // namespace nc
