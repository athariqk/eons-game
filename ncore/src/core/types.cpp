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

    const_cast<TypeInfo*>( TypeRegistry::find<float>() )->is_floating  = true;
    const_cast<TypeInfo*>( TypeRegistry::find<double>() )->is_floating = true;

    TypeRegistry::register_type<size_t>( "size_t" );
    TypeRegistry::register_type<uint8_t>( "uint8_t" );
    TypeRegistry::register_type<StringClass, nc::String>( "nc::String" );
    TypeRegistry::register_type<VectorClass<nc::DynamicArray<int>>, nc::DynamicArray<int>>( "nc::DynamicArray<int>" );

    // TODO: should this be here?
    TypeRegistry::register_type<TRecordInfo<nc::NcObject>, nc::NcObject>( "nc::NcObject" );

    // Manually register RID (rid.h is included before NSTRUCT is available)
    auto& rid_info              = TypeRegistry::register_type<TRecordInfo<nc::RID>, nc::RID>( "nc::RID" );
    static FieldInfo rid_flds[] = {
        { "value", detail::type_id<uint64_t>(), sizeof( uint64_t ), offsetof( nc::RID, value ),
          PropertyFlags::SERIALIZABLE | PropertyFlags::EDITABLE, detail::category_of<uint64_t>(), Qualifier{} },
    };
    rid_info.fields_begin = rid_flds;
    rid_info.fields_end   = rid_flds + 1;

    for (auto* c = type_list_head; c; c = c->_next) {
        get_instance().type_cache[c->id] = c;
    }

    NC_LOG_DEBUG( "TypeRegistry initialized" );
}

void TypeRegistry::shutdown()
{
    get_instance().type_cache.clear();
    NC_LOG_DEBUG( "TypeRegistry shutdown" );
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
        if (f.qualifier.is_array)
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

    if (q.is_array)
        visitor->array_element( t, depth, array_elem );
    else
        visitor->class_member( field, depth );

    if (t->is_record()) {
        auto* c = static_cast<const RecordInfo*>( t );
        if (q.is_pointer) {
            auto* p = *static_cast<void const* const*>( ptr );
            if (p)
                c->visit( p, visitor, filter, static_cast<unsigned>( depth ) );
        } else {
            c->visit( ptr, visitor, filter, static_cast<unsigned>( depth ) );
        }
    } else {
        if (field->category == FieldCategory::STRING)
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
        cursor += field->width;
    }

    visitor->array_end( t, static_cast<int>( depth ) );
}

// ======================================================================
// to_string
// ======================================================================

void TypeInfo::to_string( String& out, const void* instance ) const
{
    if (!instance) {
        out += "null";
        return;
    }

    switch (category) {
        case FieldCategory::SCALAR:
            if (is_floating) {
                if (size == 4)
                    out += std::format( "{}", *static_cast<const float*>( instance ) );
                if (size == 8)
                    out += std::format( "{}", *static_cast<const double*>( instance ) );
            }
            if (size == 1)
                out += std::format( "{}", *static_cast<const int8_t*>( instance ) );
            if (size == 2)
                out += std::format( "{}", *static_cast<const int16_t*>( instance ) );
            if (size == 4)
                out += std::format( "{}", *static_cast<const int32_t*>( instance ) );
            if (size == 8)
                out += std::format( "{}", *static_cast<const int64_t*>( instance ) );
            break;
        case FieldCategory::POINTER:
            out += std::format( "{}", *static_cast<const void* const*>( instance ) );
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
        f.value_to_string( out, instance );
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

void FieldInfo::value_to_string( String& out, const void* instance ) const
{
    auto* field_ptr = get_void_ptr( instance );
    auto* type      = get_type();

    if (type)
        return type->to_string( out, field_ptr );

    switch (category) {
        case FieldCategory::SCALAR:
            if (width == 4)
                out += std::format( "{}", *static_cast<const float*>( field_ptr ) );
            if (width == 8)
                out += std::format( "{}", *static_cast<const double*>( field_ptr ) );
            if (width == 1)
                out += std::format( "{}", *static_cast<const uint8_t*>( field_ptr ) );
            if (width == 2)
                out += std::format( "{}", *static_cast<const uint16_t*>( field_ptr ) );
            if (width == 4)
                out += std::format( "{}", *static_cast<const uint32_t*>( field_ptr ) );
            if (width == 8)
                out += std::format( "{}", *static_cast<const uint64_t*>( field_ptr ) );
            break;
        case FieldCategory::STRING:
            out += std::format( "\"{}\"", *static_cast<const char* const*>( field_ptr ) );
            break;
        case FieldCategory::POINTER:
            out += std::format( "{}", *static_cast<const void* const*>( field_ptr ) );
            break;
        default:
            out += std::format( "{}", field_ptr );
            break;
    }
}

} // namespace nc::rtti
