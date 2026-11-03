// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <ncore/object/object_info.h>

namespace nc {

bool Object::is_a( types::TypeId target ) const
{
    for (const types::RecordInfo* r = &get_class_info(); r; r = r->get_parent()) {
        if (r->id == target)
            return true;
    }
    return false;
}

// -----------------------------------------------------------------------------

void RefCounted::increment_ref()
{
    ref_count.fetch_add( 1, std::memory_order_relaxed );
}

bool RefCounted::decrement_ref()
{
    if (ref_count.fetch_sub( 1, std::memory_order_release ) == 1) {
        std::atomic_thread_fence( std::memory_order_acquire );
        return true;
    }

    return false;
}

uint32_t RefCounted::get_ref_count() const
{
    return ref_count.load( std::memory_order_relaxed );
}

const StringView RefCounted::get_class_name() const
{
    return "RefCounted";
}

types::TypeId RefCounted::get_type_id() const
{
    return TypeDB::get_type_id<RefCounted>();
}

const types::RecordInfo& RefCounted::get_class_info() const
{
    return TypeDB::get_record<RefCounted>();
}

} // namespace nc
