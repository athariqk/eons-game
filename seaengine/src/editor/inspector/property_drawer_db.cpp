// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: property_drawer_db.cpp

#include "property_drawer_db.h"

#include <algorithm>

namespace sea::editor {

void PropertyDrawerDB::register_drawer( const nc::types::TypeInfo* type, PropertyDrawerFn drawer )
{
    if (!type || !drawer)
        return;

    const auto it =
        std::find_if( entries.begin(), entries.end(), [type]( const Entry& entry ) { return entry.type == type; } );

    if (it != entries.end()) {
        it->drawer = drawer;
        return;
    }

    entries.push_back( Entry{ type, drawer } );
}

bool PropertyDrawerDB::unregister_drawer( const nc::types::TypeInfo* type )
{
    const auto old_size = entries.size();
    std::erase_if( entries, [type]( const Entry& entry ) { return entry.type == type; } );
    return entries.size() != old_size;
}

PropertyDrawerFn PropertyDrawerDB::find( const nc::types::TypeInfo* type ) const noexcept
{
    if (!type)
        return nullptr;

    const auto it =
        std::find_if( entries.begin(), entries.end(), [type]( const Entry& entry ) { return entry.type == type; } );

    return it != entries.end() ? it->drawer : nullptr;
}

void PropertyDrawerDB::clear() noexcept
{
    entries.clear();
}

} // namespace sea::editor
