// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: property_draw_context.h

#pragma once

#include <ncore/types/variant.h>

namespace sea::editor {

struct EditorFrame;
class EditorAssets;

/**
 * Context passed to a registered property drawer.
 *
 * `value` is always available for reading. `editable` is empty when the source
 * is const or the field is read-only. TypedPtr is non-owning: the inspected
 * object must outlive the draw call (and any retained view must not escape it).
 */
struct PropertyDrawContext {
    nc::ConstTypedPtr value{};
    nc::TypedPtr editable{};

    const nc::types::FieldInfo* field  = nullptr;
    const nc::types::RecordInfo* owner = nullptr;

    nc::StringView label{};
    int depth      = 0;
    bool read_only = false;

    EditorFrame* frame   = nullptr; // per-frame
    EditorAssets* assets = nullptr; // stable

    bool can_edit() const noexcept
    {
        return !read_only && static_cast<bool>( editable );
    }

    template<typename T>
    const T* get() const noexcept
    {
        return value.template try_as<T>();
    }

    template<typename T>
    T* edit() const noexcept
    {
        return can_edit() ? editable.try_as<T>() : nullptr;
    }
};

using PropertyDrawerFn = bool ( * )( PropertyDrawContext& );

} // namespace sea::editor
