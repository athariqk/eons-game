// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <ncore/runtime/resources/font.h>

namespace nc {

ResourceFormatID Font::get_format_id() const
{
    return "nfnt";
}

Span<const std::byte> Font::get_data() const
{
    return data;
}

void Font::set_data( const void* p_data, size_t p_size )
{
    auto* p = static_cast<const std::byte*>( p_data );
    data.assign( p, p + p_size );
}

size_t Font::get_size_bytes() const
{
    return data.size();
}

} // namespace nc
