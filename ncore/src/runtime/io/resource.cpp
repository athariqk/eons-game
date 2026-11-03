// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: resource.cpp

#include <ncore/runtime/io/resource.h>
#include <ncore/utils/four_cc.h>

namespace nc {

ResourceFormatID::ResourceFormatID( StringView ascii_id )
{
    id = FourCC::from_string( ascii_id );
}

ResourceFormatID::ResourceFormatID( const char* ascii_id )
{
    id = FourCC::from_string( ascii_id );
}

bool ResourceFormatID::is_valid() const
{
    return id != 0 && FourCC::is_valid_fourcc( id );
}

String ResourceFormatID::to_string() const
{
    return is_valid() ? FourCC::to_string( id ) : String();
}

// -------------------------------------------------------------------------

ResourceFormatID IResource::get_format_id() const
{
    return ResourceFormatID();
}

uint32_t IResource::get_version() const
{
    return 1;
}

size_t IResource::get_size_bytes() const
{
    return 0;
}

RID IResource::get_rid() const
{
    return prepared_rid;
}

uint32_t IResource::get_revision() const
{
    return revision;
}

RID IResource::prepare( Scene& p_ctx )
{
    ( void ) p_ctx;
    return prepared_rid;
}

void IResource::set_prepared( RID p_rid, uint32_t p_service_revision )
{
    prepared_rid     = p_rid;
    service_revision = p_service_revision;
    ++revision;
}

} // namespace nc
