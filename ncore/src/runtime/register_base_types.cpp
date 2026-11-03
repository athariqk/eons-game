#include <ncore/object/object_info.h>
#include <ncore/runtime/register_base_types.h>
#include <ncore/types/classinfo.h>
#include <ncore/types/typedb.h>

namespace nc {

void NCAPI register_base_types()
{
    TypeDB::register_type<bool>( "bool" );
    TypeDB::register_type<int>( "int" );
    TypeDB::register_type<unsigned int>( "unsigned int" );
    TypeDB::register_type<long>( "long" );
    TypeDB::register_type<unsigned long>( "unsigned long" );
    TypeDB::register_type<long long>( "long long" );
    TypeDB::register_type<unsigned long long>( "unsigned long long" );
    TypeDB::register_type<float>( "float" );
    TypeDB::register_type<double>( "double" );
    TypeDB::register_type<char>( "char" );
    TypeDB::register_type<unsigned char>( "unsigned char" );
    TypeDB::register_type<short>( "short" );
    TypeDB::register_type<unsigned short>( "unsigned short" );
    // std::byte is an enumerator-less scoped enum; reflection_gen skips empty
    // enums, but BytesBuffer (std::vector<std::byte>) elements need a TypeId
    // for VectorClass visitation. kind_of<std::byte>() yields ENUM (1 byte).
    TypeDB::register_type<std::byte>( "std::byte" );
    TypeDB::register_type<types::StringClass, nc::String>( "nc::String" );
    TypeDB::register_type<types::ObjectClass<Object>, Object>( "nc::Object" );
    TypeDB::register_type<types::ObjectClass<RefCounted>, RefCounted>( "nc::RefCounted" );
}

} // namespace nc
