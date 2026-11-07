// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <ncore/types/typedb.h>

namespace nc::types {

namespace {

template<typename V>
V read_as( const void* instance )
{
    V v;
    std::memcpy( &v, instance, sizeof( V ) );
    return v;
}

} // namespace

// ======================================================================
// TypeInfo
// ======================================================================

void TypeInfo::construct( void* instance, const void* data ) const
{
    if (data)
        std::memcpy( instance, data, size );
    else
        std::memset( instance, 0, size );
}

void TypeInfo::destruct( void* ) const {}

void TypeInfo::clone( const void* src, void* dst ) const
{
    std::memcpy( dst, src, size );
}

void TypeInfo::replace( const void* src, void* dst ) const
{
    std::memcpy( dst, src, size );
}

void TypeInfo::to_string( String& out, const void* instance ) const
{
    if (!instance) {
        out += "null";
        return;
    }

    switch (kind) {
        case TypeKind::BOOL:
            out += read_as<bool>( instance ) ? "true" : "false";
            break;
        case TypeKind::INT8:
            out += std::format( "{}", read_as<int8_t>( instance ) );
            break;
        case TypeKind::UINT8:
            out += std::format( "{}", read_as<uint8_t>( instance ) );
            break;
        case TypeKind::INT16:
            out += std::format( "{}", read_as<int16_t>( instance ) );
            break;
        case TypeKind::UINT16:
            out += std::format( "{}", read_as<uint16_t>( instance ) );
            break;
        case TypeKind::INT32:
            out += std::format( "{}", read_as<int32_t>( instance ) );
            break;
        case TypeKind::UINT32:
            out += std::format( "{}", read_as<uint32_t>( instance ) );
            break;
        case TypeKind::INT64:
            out += std::format( "{}", read_as<int64_t>( instance ) );
            break;
        case TypeKind::UINT64:
            out += std::format( "{}", read_as<uint64_t>( instance ) );
            break;
        case TypeKind::FLOAT:
            out += std::format( "{}", read_as<float>( instance ) );
            break;
        case TypeKind::DOUBLE:
            out += std::format( "{}", read_as<double>( instance ) );
            break;
        default:
            out += std::format( "{}", instance );
            break;
    }
}

// ======================================================================
// FieldInfo
// ======================================================================

const TypeInfo* FieldInfo::get_type() const
{
    return TypeDB::find( type_id );
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlifetime-safety-invalidation"
void FieldInfo::to_string( String& out, const void* instance ) const
{
    auto field_ptr = get_void_ptr( instance );
    auto& q        = qualifier;

    if (q.is_cstring) {
        out += std::format( "\"{}\"", *static_cast<const char* const*>( field_ptr ) );
        return;
    }

    auto type = get_type();
    if (!type) {
        out += "<unregistered>";
        return;
    }

    if (q.is_pointer()) {
        out += std::format( "{}", *static_cast<const void* const*>( field_ptr ) );
        return;
    }

    if (q.is_array()) {
        auto* cursor = static_cast<const uint8_t*>( field_ptr );
        out += "[";
        for (unsigned i = 0; i < q.array_length; ++i) {
            if (i)
                out += ", ";
            type->to_string( out, cursor );
            cursor += type->size;
        }
        out += "]";
        return;
    }

    type->to_string( out, field_ptr );
}
#pragma clang diagnostic pop

// ======================================================================
// RecordInfo
// ======================================================================

const RecordInfo* RecordInfo::dynamic_record( const void* ) const noexcept
{
    return this;
}

RefCounted* RecordInfo::create_refcounted() const
{
    return nullptr;
}

const FieldInfo* RecordInfo::find_field( StringView n ) const noexcept
{
    for (auto& f : fields())
        if (f.name == n)
            return &f;
    return nullptr;
}

void RecordInfo::visit( void* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept
{
    if (!instance) {
        visitor->primitive( this, nullptr );
        return;
    }

    // walk up the parent chain first.
    if (resolved_parent) {
        resolved_parent->visit( instance, visitor, filter, depth );
    }

    visitor->class_begin( this, static_cast<int>( depth ) );
    for (auto& f : fields()) {
        auto* ptr = f.get_void_ptr( instance );
        if (f.qualifier.is_array()) {
            visit_array( ptr, &f, visitor, filter, depth + 1 );
        } else {

            visit_field( ptr, &f, visitor, filter, static_cast<int>( depth + 1 ) );
        }
    }
    visitor->class_end( this, static_cast<int>( depth ) );
}

void RecordInfo::visit_field(
    void* ptr, const FieldInfo* field, RecordVisitor* visitor, PropertyFlags filter, int depth, int array_elem
) const noexcept
{
    if (!has_any_flag( field->flags, filter ))
        return;

    auto& q = field->qualifier;
    auto t  = field->get_type();

    if (!t)
        return;

    if (q.is_array()) {
        visitor->array_element( t, depth, array_elem );
    } else {
        visitor->class_member( field, depth );
    }

    if (t->is_record()) {
        auto* c = static_cast<const RecordInfo*>( t );
        if (q.is_pointer()) {
            auto* p = *static_cast<void**>( ptr );
            if (p)
                c->visit( p, visitor, filter, static_cast<unsigned>( depth ) );
        } else if (c->is_ref()) {
            // Handles are never serialized as raw pointer bytes. Generalized visitors
            // ignore them; archives persist a path/RID instead.
            visitor->reference( c, ptr );
        } else {
            c->visit( ptr, visitor, filter, static_cast<unsigned>( depth ) );
        }
    } else {
        if (t->is_string() || q.is_cstring) {
            visitor->string( t, ptr );
        } else {
            visitor->primitive( t, ptr );
        }
    }
}

void RecordInfo::visit_array(
    void* ptr, FieldInfo const* field, RecordVisitor* visitor, PropertyFlags filter, unsigned depth
) const noexcept
{
    if (!has_any_flag( field->flags, filter ))
        return;

    auto& q = field->qualifier;
    auto t  = field->get_type();

    visitor->class_member( field, static_cast<int>( depth ) );
    visitor->array_begin( t, this, ptr, static_cast<int>( depth ), static_cast<int>( q.array_length ) );

    auto* cursor = static_cast<uint8_t*>( ptr );
    for (unsigned i = 0; i < q.array_length; ++i) {
        visit_field( cursor, field, visitor, filter, static_cast<int>( depth + 1 ), static_cast<int>( i ) );
        cursor += t->size;
    }

    visitor->array_end( t, static_cast<int>( depth ) );
}

void RecordInfo::resize( void* instance, size_t length ) const noexcept
{
    // Fixed-size arrays and plain records are already sized correctly after
    // construction; readers just validate the stream count against
    // array_length. Only VectorClass needs a real resize.
    ( void ) instance;
    ( void ) length;
}

size_t RecordInfo::container_size( const void* ) const noexcept
{
    return 0;
}

void* RecordInfo::container_data( void* ) const noexcept
{
    return nullptr;
}

const TypeInfo* RecordInfo::element_type() const noexcept
{
    return nullptr;
}

void RecordInfo::construct( void* instance, const void* data ) const
{
    for (auto& f : fields()) {
        auto* dst = static_cast<uint8_t*>( f.get_void_ptr( instance ) );
        // Pointer fields: only the pointer slot belongs to this object.
        // Never construct the pointee in-place — f.width is sizeof(T*),
        // not sizeof(*T) (constructing a large class here overflows the parent).
        if (f.qualifier.is_pointer()) {
            if (data)
                std::memcpy( dst, f.get_void_ptr( data ), f.width );
            else
                std::memset( dst, 0, f.width );
            continue;
        }
        auto* t = f.get_type();
        if (!t)
            continue;
        if (f.qualifier.is_array()) {
            const uint8_t* src = data ? static_cast<const uint8_t*>( f.get_void_ptr( data ) ) : nullptr;
            for (uint32_t i = 0; i < f.qualifier.array_length; ++i) {
                t->construct( dst, src );
                dst += f.width;
                if (src)
                    src += f.width;
            }
        } else {
            t->construct( dst, data ? f.get_void_ptr( data ) : nullptr );
        }
    }
}

void RecordInfo::destruct( void* instance ) const
{
    for (auto& f : fields()) {
        // Raw pointers are non-owning — nothing to destroy.
        if (f.qualifier.is_pointer())
            continue;
        auto* t = f.get_type();
        if (!t)
            continue;
        auto* ptr = static_cast<uint8_t*>( f.get_void_ptr( instance ) );
        if (f.qualifier.is_array()) {
            for (uint32_t i = 0; i < f.qualifier.array_length; ++i) {
                t->destruct( ptr );
                ptr += f.width;
            }
        } else {
            t->destruct( ptr );
        }
    }
}

void RecordInfo::clone( const void* src, void* dst ) const
{
    for (auto& f : fields()) {
        auto* fsrc = static_cast<const uint8_t*>( f.get_void_ptr( src ) );
        auto* fdst = static_cast<uint8_t*>( f.get_void_ptr( dst ) );
        if (f.qualifier.is_pointer()) {
            std::memcpy( fdst, fsrc, f.width );
            continue;
        }
        auto* t = f.get_type();
        if (!t)
            continue;
        if (f.qualifier.is_array()) {
            for (uint32_t i = 0; i < f.qualifier.array_length; ++i) {
                t->clone( fsrc, fdst );
                fsrc += f.width;
                fdst += f.width;
            }
        } else {
            t->clone( fsrc, fdst );
        }
    }
}

void RecordInfo::replace( const void* src, void* dst ) const
{
    for (auto& f : fields()) {
        auto* fsrc = static_cast<const uint8_t*>( f.get_void_ptr( src ) );
        auto* fdst = static_cast<uint8_t*>( f.get_void_ptr( dst ) );
        if (f.qualifier.is_pointer()) {
            std::memcpy( fdst, fsrc, f.width );
            continue;
        }
        auto* t = f.get_type();
        if (!t)
            continue;
        if (f.qualifier.is_array()) {
            for (uint32_t i = 0; i < f.qualifier.array_length; ++i) {
                t->replace( fsrc, fdst );
                fsrc += f.width;
                fdst += f.width;
            }
        } else {
            t->replace( fsrc, fdst );
        }
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlifetime-safety-invalidation"
void RecordInfo::to_string( String& out, const void* instance ) const
{
    if (!instance) {
        out += String( name ) + "(null)";
        return;
    }

    out += name;
    out += "(";
    for (size_t i = 0; i < field_count(); ++i) {
        auto& f = fields()[i];
        if (i != 0)
            out += ", ";
        out += f.name;
        out += "=";
        f.to_string( out, instance );
    }
    out += ")";
}
#pragma clang diagnostic pop

// ======================================================================
// EnumInfo
// ======================================================================

int64_t EnumInfo::get_value( const void* instance ) const noexcept
{
    if (!instance)
        return 0;
    switch (size) {
        case 1: {
            uint8_t u = 0;
            std::memcpy( &u, instance, 1 );
            return is_unsigned ? static_cast<int64_t>( u )
                               : static_cast<int64_t>( static_cast<int8_t>( u ) );
        }
        case 2: {
            uint16_t u = 0;
            std::memcpy( &u, instance, 2 );
            return is_unsigned ? static_cast<int64_t>( u )
                               : static_cast<int64_t>( static_cast<int16_t>( u ) );
        }
        case 4: {
            uint32_t u = 0;
            std::memcpy( &u, instance, 4 );
            return is_unsigned ? static_cast<int64_t>( u )
                               : static_cast<int64_t>( static_cast<int32_t>( u ) );
        }
        case 8: {
            uint64_t u = 0;
            std::memcpy( &u, instance, 8 );
            return is_unsigned ? static_cast<int64_t>( u )
                               : static_cast<int64_t>( static_cast<int64_t>( u ) );
        }
        default:
            return 0;
    }
}

void EnumInfo::set_value( void* instance, int64_t value ) const noexcept
{
    switch (size) {
        case 1: {
            uint8_t v = static_cast<uint8_t>( value );
            std::memcpy( instance, &v, sizeof( v ) );
            break;
        }
        case 2: {
            uint16_t v = static_cast<uint16_t>( value );
            std::memcpy( instance, &v, sizeof( v ) );
            break;
        }
        case 4: {
            uint32_t v = static_cast<uint32_t>( value );
            std::memcpy( instance, &v, sizeof( v ) );
            break;
        }
        case 8: {
            uint64_t v = static_cast<uint64_t>( value );
            std::memcpy( instance, &v, sizeof( v ) );
            break;
        }
        default:
            break;
    }
}

void EnumInfo::to_string( String& out, const void* instance ) const
{
    if (!instance) {
        out += "null";
        return;
    }

    out += get_name( get_value( instance ) );
}

} // namespace nc::types
