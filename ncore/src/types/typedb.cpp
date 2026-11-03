// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <ncore/types/typedb.h>
#include <ncore/utils/assert.h>

namespace nc {

// ======================================================================
// TypeDB
// ======================================================================

types::TypeInfo* TypeDB::type_list_head = nullptr;
int TypeDB::type_query_hits             = 0;

TypeDB::TypeDB() = default;

const types::TypeInfo& TypeDB::get( types::TypeId id ) noexcept
{
    auto* p = find( id );
    NC_ASSERT_MSG( p != nullptr, "Type not registered" );
    return *p;
}

const types::TypeInfo& TypeDB::get( StringView name ) noexcept
{
    auto* p = find( name );
    NC_ASSERT_MSG( p != nullptr, "Type not registered" );
    return *p;
}

void TypeDB::initialize()
{
    // Seed any ids not already cached. Do not overwrite: NSTRUCT_V/NCLASS/
    // ECS_COMPONENT always writes type_cache during static init and must beat
    // a plain gen RecordInfo for the same TypeId (list order between TUs is
    // unspecified).
    auto& cache = get_instance().type_cache;
    for (auto* c = type_list_head; c; c = c->next) {
        if (cache.find( c->id ) == cache.end())
            cache[c->id] = c;
    }

    // Apply provide_fields() ranges that arrived before their TypeId was
    // registered (gen static-init vs header TU order is unspecified).
    auto& pending = get_instance().pending_fields;
    if (!pending.empty()) {
        for (auto& [id, range] : pending) {
            auto it = cache.find( id );
            if (it == cache.end())
                continue;
            if (auto* rec = dynamic_cast<types::RecordInfo*>( it->second ); rec && !rec->fields_begin && range.first) {
                rec->fields_begin = range.first;
                rec->fields_end   = range.second;
            }
        }
        pending.clear();
    }

    for (auto* c = type_list_head; c; c = c->next) {
        if (!c->is_record())
            continue;
        auto* r = static_cast<types::RecordInfo*>( c );
        if (r->parent_id.valid())
            r->resolved_parent = find_record( r->parent_id );
    }
}

void TypeDB::shutdown()
{
    get_instance().type_cache.clear();
    get_instance().pending_fields.clear();
}

void TypeDB::register_type( types::TypeInfo* p_info ) noexcept
{
    if (!p_info)
        return;
    p_info->next   = type_list_head;
    type_list_head = p_info;
    // Gen-emitted plain RecordInfo must not clobber a ObjectClass already
    // registered via NSTRUCT_V/NCLASS/ECS_COMPONENT under the same TypeId
    // (static-init order between reflection_impl.cpp and header TUs is
    // unspecified). Those always overwrite type_cache; we only seed when the
    // id is absent.
    auto& cache = get_instance().type_cache;
    if (cache.find( p_info->id ) == cache.end())
        cache[p_info->id] = p_info;
}

void TypeDB::apply_pending_fields( types::TypeInfo& info ) noexcept
{
    auto& pending = get_instance().pending_fields;
    auto it       = pending.find( info.id );
    if (it == pending.end())
        return;
    auto* rec = dynamic_cast<types::RecordInfo*>( &info );
    if (rec && !rec->fields_begin && it->second.first) {
        rec->fields_begin = it->second.first;
        rec->fields_end   = it->second.second;
    }
    pending.erase( it );
}

void TypeDB::provide_fields( types::TypeId id, const types::FieldInfo* begin, const types::FieldInfo* end ) noexcept
{
    if (!id.valid() || !begin)
        return;

    auto& inst = get_instance();

    // Prefer attaching to the current cache winner (ObjectClass or plain).
    auto it = inst.type_cache.find( id );
    if (it != inst.type_cache.end()) {
        if (auto* rec = dynamic_cast<types::RecordInfo*>( it->second ); rec && !rec->fields_begin) {
            rec->fields_begin = begin;
            rec->fields_end   = end;
            return;
        }
        // Winner already has fields (NPROPS or earlier provide_fields) — keep them.
        return;
    }

    // Not cached yet: walk the list in case a RecordInfo is linked but not cached.
    for (auto* c = type_list_head; c; c = c->next) {
        if (c->id != id)
            continue;
        if (auto* rec = dynamic_cast<types::RecordInfo*>( c ); rec && !rec->fields_begin) {
            rec->fields_begin   = begin;
            rec->fields_end     = end;
            inst.type_cache[id] = rec;
            return;
        }
        return;
    }

    // Defer until register_type/initialize creates or caches the type.
    inst.pending_fields[id] = { begin, end };
}

} // namespace nc
