// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: utils.h

#pragma once

#include <ncore/core/collection.h>

namespace sea::editor::utils {

static void pretty_name( nc::StringView p_raw, char* p_out, size_t p_cap )
{
    if (!p_out || p_cap == 0)
        return;
    size_t start    = 0;
    const size_t ns = p_raw.rfind( "::" );
    if (ns != nc::StringView::npos)
        start = ns + 2;
    if (p_raw.size() - start > 2 && p_raw[start] == 'm' && p_raw[start + 1] == '_')
        start += 2;

    size_t o      = 0;
    bool cap_next = true;
    char prev     = 0;
    for (size_t i = start; i < p_raw.size() && o + 2 < p_cap; ++i) {
        const char c = p_raw[i];
        if (c == '_' || c == ' ') {
            if (o > 0 && p_out[o - 1] != ' ')
                p_out[o++] = ' ';
            cap_next = true;
            prev     = c;
            continue;
        }
        if (std::isupper( static_cast<unsigned char>( c ) ) && o > 0 &&
            ( std::islower( static_cast<unsigned char>( prev ) ) ||
              std::isdigit( static_cast<unsigned char>( prev ) ) ))
            p_out[o++] = ' ';
        p_out[o++] = cap_next ? static_cast<char>( std::toupper( static_cast<unsigned char>( c ) ) ) : c;
        cap_next   = false;
        prev       = c;
    }
    p_out[o] = '\0';
}

} // namespace sea::editor::utils
