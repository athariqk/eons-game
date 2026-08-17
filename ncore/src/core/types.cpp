#include <ncore/core/collection.h>
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
    NC_ASSERT( p != nullptr, "Type not registered" );
    return *p;
}

const TypeInfo& TypeRegistry::get( StringView name ) noexcept
{
    auto* p = find( name );
    NC_ASSERT( p != nullptr, "Type not registered" );
    return *p;
}

void TypeRegistry::initialize()
{
    TypeRegistry::register_type<bool>( "bool" );
    TypeRegistry::register_type<int32_t>( "int32_t" );
    TypeRegistry::register_type<int>( "int" );
    TypeRegistry::register_type<uint32_t>( "uint32_t" );
    TypeRegistry::register_type<int64_t>( "int64_t" );
    TypeRegistry::register_type<uint64_t>( "uint64_t" );
    TypeRegistry::register_type<float>( "float" );
    TypeRegistry::register_type<double>( "double" );

    TypeRegistry::register_type<size_t>( "size_t" );
    TypeRegistry::register_type<uint8_t>( "uint8_t" );
    TypeRegistry::register_type<char>( "char" );
    TypeRegistry::register_type<StringClass, nc::String>( "nc::String" );
    TypeRegistry::register_type<VectorClass<nc::DynamicArray<int>>, nc::DynamicArray<int>>( "nc::DynamicArray<int>" );
    TypeRegistry::register_type<VectorClass<nc::Array<RID, 8>>, nc::Array<RID, 8>>(
        "nc::Array<RID, 8>"
    ); // HACK: temporary placeholder, remove when we got proper auto-reflection

    // TODO: should this be here?
    TypeRegistry::register_type<TRecordInfo<nc::NcObject>, nc::NcObject>( "nc::NcObject" );

    // Manually register RID (rid.h is included before NSTRUCT is available)
    auto& rid_info              = TypeRegistry::register_type<TRecordInfo<nc::RID>, nc::RID>( "nc::RID" );
    static FieldInfo rid_flds[] = {
        { "value", detail::type_id<uint64_t>(), sizeof( uint64_t ), offsetof( nc::RID, value ),
          PropertyFlags::SERIALIZABLE | PropertyFlags::EDITABLE, Qualifier{} },
    };
    rid_info.fields_begin = rid_flds;
    rid_info.fields_end   = rid_flds + 1;

    for (auto* c = type_list_head; c; c = c->_next) {
        get_instance().type_cache[c->id] = c;
    }
}

void TypeRegistry::shutdown()
{
    get_instance().type_cache.clear();
}

// ======================================================================
// FieldInfo
// ======================================================================

const TypeInfo* FieldInfo::get_type() const
{
    return TypeRegistry::find( type_id );
}

// ======================================================================
// RecordInfo::visit
// ======================================================================

void RecordInfo::visit(
    const void* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth
) const noexcept
{
    if (!instance) {
        visitor->primitive( this, nullptr );
        return;
    }

    visitor->class_begin( this, static_cast<int>( depth ) );
    for (auto& f : fields()) {
        auto* ptr = f.get_void_ptr( instance );
        if (f.qualifier.is_array())
            visit_array( ptr, &f, visitor, filter, depth + 1 );
        else
            visit_field( ptr, &f, visitor, filter, static_cast<int>( depth + 1 ) );
    }
    visitor->class_end( this, static_cast<int>( depth ) );
}

// ======================================================================
// RecordInfo::visit_field
// ======================================================================

void RecordInfo::visit_field(
    const void* ptr, const FieldInfo* field, RecordVisitor* visitor, PropertyFlags filter, int depth, int array_elem
) const noexcept
{
    if (!has_any_flag( field->flags, filter ))
        return;

    auto& q = field->qualifier;
    auto t  = field->get_type();

    if (!t)
        return;

    if (q.is_array())
        visitor->array_element( t, depth, array_elem );
    else
        visitor->class_member( field, depth );

    if (t->is_record()) {
        auto* c = static_cast<const RecordInfo*>( t );
        if (q.is_pointer()) {
            auto* p = *static_cast<void const* const*>( ptr );
            if (p)
                c->visit( p, visitor, filter, static_cast<unsigned>( depth ) );
        } else {
            c->visit( ptr, visitor, filter, static_cast<unsigned>( depth ) );
        }
    } else {
        if (t->is_string() || q.is_cstring)
            visitor->string( t, ptr );
        else
            visitor->primitive( t, ptr );
    }
}

// ======================================================================
// RecordInfo::visit_array
// ======================================================================

void RecordInfo::visit_array(
    const void* ptr, FieldInfo const* field, RecordVisitor* visitor, PropertyFlags filter, unsigned depth
) const noexcept
{
    if (!has_any_flag( field->flags, filter ))
        return;

    auto& q = field->qualifier;
    auto t  = field->get_type();

    visitor->class_member( field, static_cast<int>( depth ) );
    visitor->array_begin( t, static_cast<int>( depth ), static_cast<int>( q.array_length ) );

    auto* cursor = static_cast<const uint8_t*>( ptr );
    for (unsigned i = 0; i < q.array_length; ++i) {
        visit_field( cursor, field, visitor, filter, static_cast<int>( depth + 1 ), static_cast<int>( i ) );
        cursor += t->size;
    }

    visitor->array_end( t, static_cast<int>( depth ) );
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

void StringClass::to_string( String& out, const void* instance ) const
{
    if (!instance) {
        out += "null";
        return;
    }

    auto* str = static_cast<const String*>( instance );
    out += std::format( "\"{}\"", *str );
}

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
