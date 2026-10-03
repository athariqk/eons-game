#include <cstddef>
#include <cstdint>
#include <cstring>

#include <ncore/core/collection.h>
#include <ncore/core/object.h>
#include <ncore/core/types.h>
#include <ncore/utils/assert.h>

namespace nc::rtti {

// ======================================================================
// TypeRegistry
// ======================================================================

TypeInfo* TypeRegistry::type_list_head = nullptr;
int TypeRegistry::rtti_hits_           = 0;

TypeRegistry::TypeRegistry() = default;

const TypeInfo& TypeRegistry::get( TypeId id ) noexcept
{
    auto* p = find( id );
    NC_ASSERT_MSG( p != nullptr, "Type not registered" );
    return *p;
}

const TypeInfo& TypeRegistry::get( StringView name ) noexcept
{
    auto* p = find( name );
    NC_ASSERT_MSG( p != nullptr, "Type not registered" );
    return *p;
}

void TypeRegistry::initialize()
{
    TypeRegistry::register_type<bool>( "bool" );
    TypeRegistry::register_type<int>( "int" );
    TypeRegistry::register_type<unsigned int>( "unsigned int" );
    TypeRegistry::register_type<long>( "long" );
    TypeRegistry::register_type<unsigned long>( "unsigned long" );
    TypeRegistry::register_type<long long>( "long long" );
    TypeRegistry::register_type<unsigned long long>( "unsigned long long" );
    TypeRegistry::register_type<float>( "float" );
    TypeRegistry::register_type<double>( "double" );
    TypeRegistry::register_type<char>( "char" );
    TypeRegistry::register_type<unsigned char>( "unsigned char" );
    TypeRegistry::register_type<short>( "short" );
    TypeRegistry::register_type<unsigned short>( "unsigned short" );
    // std::byte is an enumerator-less scoped enum; reflection_gen skips empty
    // enums, but BytesBuffer (std::vector<std::byte>) elements need a TypeId
    // for VectorClass visitation. kind_of<std::byte>() yields ENUM (1 byte).
    TypeRegistry::register_type<std::byte>( "std::byte" );
    TypeRegistry::register_type<StringClass, nc::String>( "nc::String" );
    TypeRegistry::register_type<RecordInfo, nc::Object>( "nc::Object" ); // TODO: should this be here?

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
    auto& pending = get_instance().pending_fields_;
    if (!pending.empty()) {
        for (auto& [id, range] : pending) {
            auto it = cache.find( id );
            if (it == cache.end())
                continue;
            if (auto* rec = dynamic_cast<RecordInfo*>( it->second ); rec && !rec->fields_begin && range.first) {
                rec->fields_begin = range.first;
                rec->fields_end   = range.second;
            }
        }
        pending.clear();
    }
}

void TypeRegistry::shutdown()
{
    get_instance().type_cache.clear();
    get_instance().pending_fields_.clear();
}

void TypeRegistry::register_type( TypeInfo* p_info ) noexcept
{
    if (!p_info)
        return;
    p_info->next   = type_list_head;
    type_list_head = p_info;
    // Gen-emitted plain RecordInfo must not clobber a RecordInfoT already
    // registered via NSTRUCT_V/NCLASS/ECS_COMPONENT under the same TypeId
    // (static-init order between reflection_impl.cpp and header TUs is
    // unspecified). Those always overwrite type_cache; we only seed when the
    // id is absent.
    auto& cache = get_instance().type_cache;
    if (cache.find( p_info->id ) == cache.end())
        cache[p_info->id] = p_info;
}

void TypeRegistry::apply_pending_fields_( TypeInfo& info ) noexcept
{
    auto& pending = get_instance().pending_fields_;
    auto it       = pending.find( info.id );
    if (it == pending.end())
        return;
    auto* rec = dynamic_cast<RecordInfo*>( &info );
    if (rec && !rec->fields_begin && it->second.first) {
        rec->fields_begin = it->second.first;
        rec->fields_end   = it->second.second;
    }
    pending.erase( it );
}

void TypeRegistry::provide_fields( TypeId id, const FieldInfo* begin, const FieldInfo* end ) noexcept
{
    if (!id.valid() || !begin)
        return;

    auto& inst = get_instance();

    // Prefer attaching to the current cache winner (RecordInfoT or plain).
    auto it = inst.type_cache.find( id );
    if (it != inst.type_cache.end()) {
        if (auto* rec = dynamic_cast<RecordInfo*>( it->second ); rec && !rec->fields_begin) {
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
        if (auto* rec = dynamic_cast<RecordInfo*>( c ); rec && !rec->fields_begin) {
            rec->fields_begin = begin;
            rec->fields_end   = end;
            inst.type_cache[id] = rec;
            return;
        }
        return;
    }

    // Defer until register_type/initialize creates or caches the type.
    inst.pending_fields_[id] = { begin, end };
}

// ======================================================================
// FieldInfo
// ======================================================================

const TypeInfo* FieldInfo::get_type() const
{
    return TypeRegistry::find( type_id );
}

// ======================================================================
// RecordInfo
// ======================================================================

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
    if (auto pt = TypeRegistry::find( parent_id )) {
        if (pt->is_record()) {
            auto pc = static_cast<const RecordInfo*>( pt );
            pc->visit( instance, visitor, filter, depth );
        }
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

// ======================================================================
// TypeInfo lifecycle (base for primitives, enums, pointers)
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

// ======================================================================
// RecordInfo lifecycle
// ======================================================================

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

// ======================================================================
// StringClass lifecycle
// ======================================================================

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

// ======================================================================
// to_string
// ======================================================================

namespace {

template<typename V>
V read_as( const void* instance )
{
    V v;
    std::memcpy( &v, instance, sizeof( V ) );
    return v;
}

} // namespace

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

void StringClass::to_string( String& out, const void* instance ) const
{
    if (!instance) {
        out += "null";
        return;
    }

    auto* str = static_cast<const String*>( instance );
    out += std::format( "\"{}\"", *str );
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
// EnumInfo
// ======================================================================

int64_t EnumInfo::get_value( const void* instance ) const noexcept
{
    switch (size) {
        case 1:
            return is_unsigned ? static_cast<int64_t>( read_as<uint8_t>( instance ) )
                               : static_cast<int64_t>( static_cast<int8_t>( read_as<uint8_t>( instance ) ) );
        case 2:
            return is_unsigned ? static_cast<int64_t>( read_as<uint16_t>( instance ) )
                               : static_cast<int64_t>( static_cast<int16_t>( read_as<uint16_t>( instance ) ) );
        case 4:
            return is_unsigned ? static_cast<int64_t>( read_as<uint32_t>( instance ) )
                               : static_cast<int64_t>( static_cast<int32_t>( read_as<uint32_t>( instance ) ) );
        case 8:
            return is_unsigned ? static_cast<int64_t>( read_as<uint64_t>( instance ) )
                               : static_cast<int64_t>( read_as<uint64_t>( instance ) );
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

} // namespace nc::rtti
