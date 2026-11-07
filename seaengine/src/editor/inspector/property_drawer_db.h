// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: property_drawer_db.h

#pragma once

#include "property_draw_context.h"

namespace sea::editor {

/**
 * @brief Exact-type drawer registry. Kept deliberately small and predictable.
 */
class PropertyDrawerDB {
public:
    void register_drawer( const nc::types::TypeInfo* type, PropertyDrawerFn drawer );
    bool unregister_drawer( const nc::types::TypeInfo* type );
    PropertyDrawerFn find( const nc::types::TypeInfo* type ) const noexcept;
    void clear() noexcept;

private:
    struct Entry {
        const nc::types::TypeInfo* type = nullptr;
        PropertyDrawerFn drawer         = nullptr;
    };

    nc::DynamicArray<Entry> entries;
};

} // namespace sea::editor
