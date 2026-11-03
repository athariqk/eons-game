// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <ncore/types/classinfo.h>

namespace nc::types {

void StringClass::construct( void* instance, const void* data ) const
{
    if (data)
        new ( instance ) String( *static_cast<const String*>( data ) );
    else
        new ( instance ) String();
}

void StringClass::destruct( void* instance ) const
{
    static_cast<String*>( instance )->~String();
}

void StringClass::clone( const void* src, void* dst ) const
{
    new ( dst ) String( *static_cast<const String*>( src ) );
}

void StringClass::replace( const void* src, void* dst ) const
{
    *static_cast<String*>( dst ) = *static_cast<const String*>( src );
}

void StringClass::to_string( String& out, const void* instance ) const
{
    if (!instance) {
        out += "null";
        return;
    }

    auto* str = static_cast<const String*>( instance );
    out += std::format( "\"{}\"", *str );
}

} // namespace nc::types
