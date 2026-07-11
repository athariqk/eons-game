// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string>

#include <ncore/kernel/object.h>
#include <ncore/kernel/reference.h>

namespace nc {

/**
 * @brief ResourceFormatID uniquely identifies a resource format (e.g. image, etc).
 */
struct ResourceFormatID {
    uint32_t id = 0;

    ResourceFormatID() = default;
    ResourceFormatID( const std::string& ascii_id );

    bool is_valid();
    std::string to_string();
};

class IResource : public RefCounted {
    NCLASS( IResource, RefCounted )

public:
    virtual ResourceFormatID get_format_id()
    {
        return ResourceFormatID();
    }

    virtual uint32_t get_version() const
    {
        return 1;
    }

    virtual size_t get_size_bytes() const
    {
        return 0;
    }

    std::string filepath;
};

} // namespace nc
