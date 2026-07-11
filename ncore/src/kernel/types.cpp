#include <ncore/kernel/object.h>
#include <ncore/kernel/types.h>
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

const TypeInfo& TypeRegistry::get( std::string_view name ) noexcept
{
    auto* p = find( name );
    NC_ASSERT( p != nullptr, "Type not registered" );
    return *p;
}

void TypeRegistry::initialize()
{
    TypeRegistry::register_type<bool>( "bool" );
    TypeRegistry::register_type<int32_t>( "int32_t" );
    TypeRegistry::register_type<uint32_t>( "uint32_t" );
    TypeRegistry::register_type<int64_t>( "int64_t" );
    TypeRegistry::register_type<uint64_t>( "uint64_t" );
    TypeRegistry::register_type<float>( "float" );
    TypeRegistry::register_type<double>( "double" );
    TypeRegistry::register_type<size_t>( "size_t" );
    TypeRegistry::register_type<uint8_t>( "uint8_t" );
    TypeRegistry::register_type<StringClass, std::string>( "std::string" );
    TypeRegistry::register_type<VectorClass<std::vector<int>>, std::vector<int>>( "std::vector<int>" );

    // TODO: should this be here?
    TypeRegistry::register_type<TRecordInfo<nc::NcObject>, nc::NcObject>( "NcObject" );

    for (auto* c = type_list_head; c; c = c->_next) {
        get_instance().type_cache[c->id] = c;
    }

	NC_LOG_TRACE( "TypeRegistry initialized" );
}

void TypeRegistry::shutdown()
{
    get_instance().type_cache.clear();
    NC_LOG_TRACE( "TypeRegistry shutdown" );
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
        if (field->category == FieldCategory::String)
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

} // namespace nc::rtti
