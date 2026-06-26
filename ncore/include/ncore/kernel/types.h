#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>

#include <ncore.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

// Core reflection types.
// Inspired by Arvid Gerstmann's metareflect
// https://github.com/Leandros/metareflect

namespace ncore::rfl {

//------------------------------------------------------------------------------

struct TypeId {
    size_t value;

    bool operator==( TypeId o ) const
    {
        return value == o.value;
    }
    bool operator!=( TypeId o ) const
    {
        return value != o.value;
    }
    bool valid() const
    {
        return value != 0;
    }
    static constexpr TypeId null()
    {
        return { 0 };
    }
};

//------------------------------------------------------------------------------

namespace detail {

constexpr size_t fnv1a( const char* s, size_t n ) noexcept
{
    size_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < n; ++i)
        h = ( h ^ static_cast<uint8_t>( s[i] ) ) * 1099511628211ULL;
    return h ? h : 1;
}

template<typename T>
constexpr size_t type_hash() noexcept
{
#if defined( __GNUC__ ) || defined( __clang__ )
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
#elif defined( _MSC_VER )
    constexpr std::string_view sig = __FUNCSIG__;
#else
#error "Unsupported compiler for stable type IDs"
#endif
    return fnv1a( sig.data(), sig.size() );
}

template<typename T>
constexpr TypeId type_id() noexcept
{
    return TypeId{ detail::type_hash<T>() };
}

} // namespace detail

//------------------------------------------------------------------------------

enum class PropertyFlags : uint16_t {
    None         = 0,
    Serializable = 1 << 0,
    Editable     = 1 << 1,
    ReadOnly     = 1 << 2,
    Hidden       = 1 << 3,
};

constexpr PropertyFlags operator|( PropertyFlags a, PropertyFlags b ) noexcept
{
    return static_cast<PropertyFlags>( static_cast<uint16_t>( a ) | static_cast<uint16_t>( b ) );
}
constexpr PropertyFlags operator&( PropertyFlags a, PropertyFlags b ) noexcept
{
    return static_cast<PropertyFlags>( static_cast<uint16_t>( a ) & static_cast<uint16_t>( b ) );
}
constexpr PropertyFlags operator~( PropertyFlags a ) noexcept
{
    return static_cast<PropertyFlags>( ~static_cast<uint16_t>( a ) );
}
constexpr PropertyFlags& operator|=( PropertyFlags& a, PropertyFlags b ) noexcept
{
    return a = a | b;
}
constexpr PropertyFlags& operator&=( PropertyFlags& a, PropertyFlags b ) noexcept
{
    return a = a & b;
}

constexpr bool has_flag( PropertyFlags f, PropertyFlags check ) noexcept
{
    return ( f & check ) != PropertyFlags::None;
}
constexpr bool has_any_flag( PropertyFlags f, PropertyFlags mask ) noexcept
{
    return ( static_cast<uint16_t>( f ) & static_cast<uint16_t>( mask ) ) != 0;
}
constexpr PropertyFlags set_flag( PropertyFlags f, PropertyFlags bit ) noexcept
{
    return f | bit;
}
constexpr PropertyFlags clear_flag( PropertyFlags f, PropertyFlags bit ) noexcept
{
    return f & ~bit;
}

//------------------------------------------------------------------------------

enum class FieldCategory : uint8_t {
    Scalar,
    String,
    Container,
    Aggregate,
    Pointer,
    Enum,
};

//------------------------------------------------------------------------------

struct Qualifier {
    unsigned array_length : 30;
    unsigned is_pointer   : 1;
    unsigned is_array     : 1;
};

//------------------------------------------------------------------------------

struct NCORE_API TypeInfo {
    const char* name;
    TypeId id;
    size_t size;
    size_t alignment;
    TypeInfo* _next = nullptr;

    TypeInfo() : name( nullptr ), id( TypeId::null() ), size( 0 ), alignment( 0 ) {}
    TypeInfo( const char* n, TypeId i, size_t sz, size_t align ) : name( n ), id( i ), size( sz ), alignment( align ) {}

    virtual ~TypeInfo() = default;

    /**
     * @brief Returns true if this type is a composite data structure (class, structs, etc).
     */
    virtual bool is_record() const noexcept
    {
        return false;
    }
};

//------------------------------------------------------------------------------

struct ContainerOps {
    size_t ( *size )( const void* );
    void* ( *at )( void*, size_t );
    void ( *insert_default )( void* );
    void ( *erase_at )( void*, size_t );
    void ( *clear )( void* );
    TypeId value_type_id;
};

//------------------------------------------------------------------------------

struct FieldInfo {
    std::string_view name;
    TypeInfo* type;
    size_t width;
    size_t offset;
    PropertyFlags flags;
    FieldCategory category;
    Qualifier qualifier;

    static constexpr unsigned FLAGS_SERIALIZED = 0x1;
    static constexpr unsigned FLAGS_CSTRING    = 0x2;

    template<typename T>
    T get_as( void* instance ) const noexcept
    {
        T ret{};
        memcpy( &ret, static_cast<uint8_t*>( instance ) + offset, sizeof( T ) );
        return ret;
    }

    template<typename T>
    T get_as( const void* instance ) const noexcept
    {
        T ret{};
        memcpy( &ret, static_cast<const uint8_t*>( instance ) + offset, sizeof( T ) );
        return ret;
    }

    template<typename T>
    T* get_ptr( void* instance ) const noexcept
    {
        return static_cast<T*>( static_cast<uint8_t*>( instance ) + offset );
    }

    template<typename T>
    const T* get_ptr( const void* instance ) const noexcept
    {
        return static_cast<const T*>( static_cast<const uint8_t*>( instance ) + offset );
    }

    void* get_void_ptr( void* instance ) const noexcept
    {
        return static_cast<uint8_t*>( instance ) + offset;
    }

    const void* get_void_ptr( const void* instance ) const noexcept
    {
        return static_cast<const uint8_t*>( instance ) + offset;
    }

    bool is( PropertyFlags f ) const noexcept
    {
        return has_flag( flags, f );
    }
};

//------------------------------------------------------------------------------

struct EnumElement {
    std::string_view name;
    int64_t value;
};

/**
 * @brief EnumInfo represents a reflected enumeration type.
 */
struct NCORE_API EnumInfo : public TypeInfo {
    const EnumElement* elements_begin = nullptr;
    const EnumElement* elements_end   = nullptr;

    EnumInfo() = default;
    EnumInfo( const char* name, TypeId t_id, size_t size, size_t align ) : TypeInfo( name, t_id, size, align ) {}

    std::span<const EnumElement> elements() const noexcept
    {
        return { elements_begin, elements_end };
    }

    bool try_get_value( std::string_view name, int64_t& out_value ) const noexcept
    {
        for (const auto& elem : elements()) {
            if (elem.name == name) {
                out_value = elem.value;
                return true;
            }
        }
        return false;
    }

    std::string_view get_name( int64_t value ) const noexcept
    {
        for (const auto& elem : elements()) {
            if (elem.value == value)
                return elem.name;
        }
        return "<unknown_enum_value>";
    }
};

//------------------------------------------------------------------------------

struct RecordVisitor;

/**
 * @brief RecordInfo represents a composite data structure (class, structs, etc).
 */
struct NCORE_API RecordInfo : public TypeInfo {
    TypeId parent_id              = TypeId::null();
    const FieldInfo* fields_begin = nullptr;
    const FieldInfo* fields_end   = nullptr;

    RecordInfo() = default;
    RecordInfo( const char* name, TypeId t_id, size_t size, size_t align ) : TypeInfo( name, t_id, size, align ) {}

    bool is_record() const noexcept override
    {
        return true;
    }

    size_t field_count() const noexcept
    {
        return fields().size();
    }

    std::span<const FieldInfo> fields() const noexcept
    {
        return { fields_begin, fields_end };
    }

    const FieldInfo* find_field( std::string_view n ) const noexcept
    {
        for (auto& f : fields())
            if (f.name == n)
                return &f;
        return nullptr;
    }

    virtual void visit(
        void const* instance, RecordVisitor* visitor, PropertyFlags filter = static_cast<PropertyFlags>( 0xFFFF ),
        unsigned depth = 0
    ) const noexcept;

    virtual void visit_field(
        void const* ptr, const FieldInfo* field, RecordVisitor* visitor, PropertyFlags filter, int depth,
        int array_elem = -1
    ) const noexcept;

    virtual void visit_array(
        void const* ptr, const FieldInfo* field, RecordVisitor* visitor, PropertyFlags filter, unsigned depth
    ) const noexcept;
};

//------------------------------------------------------------------------------

/**
 * @brief A global registry of reflected types and classes.
 * Equivalent to Godot's ClassDB.
 */
class NCORE_API Registry {
public:
    /**
     * @brief Registers a TypeInfo subclass for a given type T.
     *
     * @param TI The TypeInfo subclass to construct (e.g. RecordInfo, EnumInfo, etc).
     * @param T The actual type to reflect.
     * @param Extra Extra arguments forwarded to the TypeInfo class/subclass constructor.
     */
    template<std::derived_from<TypeInfo> TI, typename T, typename... Extra>
    static TI& emplace( const char* name, Extra&&... extra ) noexcept
    {
        static TI info( name, detail::type_id<T>(), sizeof( T ), alignof( T ), std::forward<Extra>( extra )... );
        static const bool registered = [] {
            info._next     = type_list_head;
            type_list_head = &info;
            // NC_LOG_TRACE("registered type '{}' with ID '{}'", info.name, info.id.value);
            return true;
        }();
        ( void ) registered;
        return info;
    }

    /**
     * @brief Registers a plain TypeInfo for primitives/fundamentals.
     *
     * @param T The actual type to reflect.
     * @param name The name to register the primitive type under (e.g. "int", "float", etc).
     */
    template<typename T>
    static TypeInfo& emplace( const char* name ) noexcept
    {
        return emplace<TypeInfo, T>( name );
    }

    // TODO: add register_class<T>() helper method

    static const TypeInfo* find( TypeId id ) noexcept
    {
        for (auto* c = type_list_head; c; c = c->_next)
            if (c->id == id)
                return c;
        NC_LOG_WARN( "type with ID '{}' not in the registry, probably not reflected?", id.value );
        return nullptr;
    }

    static const TypeInfo* find( std::string_view name ) noexcept
    {
        for (auto* c = type_list_head; c; c = c->_next)
            if (name == c->name)
                return c;
        NC_LOG_WARN( "type with name '{}' not in the registry, probably not reflected?", name );
        return nullptr;
    }

    static const RecordInfo* find_record( TypeId id ) noexcept
    {
        const TypeInfo* t = find( id );
        if (!t)
            return nullptr;
        if (!t->is_record()) {
            NC_LOG_WARN( "type '{}' is registered but is not a record", t->name );
            return nullptr;
        }
        return static_cast<const RecordInfo*>( t );
    }

    static const std::string to_string( void* instance, TypeId id ) noexcept
    {
        auto t = find( id );
        if (!t)
            return "<unknown type>";
        return std::format( "<{}:{}>", t->name, instance );
    }

    static const TypeInfo& get( TypeId id ) noexcept;
    static const TypeInfo& get( std::string_view name ) noexcept;

    static const std::string_view get_type_name( TypeId id ) noexcept
    {
        auto* c = find( id );
        return c ? c->name : "<unknown>";
    }

    template<typename T>
    static const TypeInfo* find() noexcept
    {
        return find( detail::type_id<T>() );
    }

    template<typename T>
    static const RecordInfo* find_record() noexcept
    {
        return find_record( detail::type_id<T>() );
    }

    template<typename T>
    static bool is_registered() noexcept
    {
        return find<T>() != nullptr;
    }

    /**
     * @brief Hard exits if we can't find the type info.
     */
    template<typename T>
    static const TypeInfo& get() noexcept
    {
        NC_ASSERT( is_registered<T>(), "type is not found in the registry" );
        return get( detail::type_id<T>() );
    }

    template<typename T>
    static const TypeId get_type_id() noexcept
    {
        return detail::type_id<T>();
    }

    template<typename T>
    static const std::string_view get_type_name()
    {
        return get_type_name( detail::type_id<T>() );
    }

    // hard-exits version, mirroring get<T>()
    template<typename T>
    static const RecordInfo& get_record() noexcept
    {
        const RecordInfo* c = find_record<T>();
        NC_ASSERT( c, "record type is not found in the registry" );
        return *c;
    }

private:
    static bool register_primitive_types();
    inline static bool _registered = register_primitive_types();

    inline static TypeInfo* type_list_head = nullptr;
};

namespace detail {

template<typename F>
constexpr FieldCategory category_of() noexcept
{
    using raw = std::remove_cvref_t<F>;
    if constexpr (std::is_pointer_v<raw>) {
        if constexpr (std::is_convertible_v<raw, std::string_view>)
            return FieldCategory::String;
        return FieldCategory::Pointer;
    } else if constexpr (std::is_convertible_v<raw, std::string_view>) {
        return FieldCategory::String;
    } else if constexpr (requires { typename raw::value_type; }) {
        return FieldCategory::Container;
    } else {
        return FieldCategory::Scalar;
    }
}

} // namespace detail

//------------------------------------------------------------------------------

struct NCORE_API RecordVisitor {
    virtual ~RecordVisitor() = default;

    virtual void class_begin( const RecordInfo* c, int depth ) = 0;
    virtual void class_end( const RecordInfo* c, int depth )   = 0;
    virtual void class_member( const FieldInfo* f, int depth ) = 0;

    virtual void array_begin( const TypeInfo* t, int depth, int length ) = 0;
    virtual void array_end( const TypeInfo* t, int depth )               = 0;
    virtual void array_element( const TypeInfo* t, int depth, int elem ) = 0;

    virtual void primitive( const TypeInfo* t, const void* instance ) = 0;
    virtual void string( const TypeInfo* t, const void* instance )    = 0;
};

//------------------------------------------------------------------------------

template<typename VecT>
struct VectorClass : public RecordInfo {
    VectorClass( const char* n, TypeId i, size_t sz, size_t align ) : RecordInfo( n, i, sz, align ) {}

    void
    visit( void const* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept override
    {
        if (!instance) {
            visitor->primitive( this, nullptr );
            return;
        }

        auto* vec       = static_cast<const VecT*>( instance );
        auto* elem_type = Registry::find<typename VecT::value_type>();

        visitor->array_begin( elem_type, static_cast<int>( depth ), static_cast<int>( vec->size() ) );
        size_t idx = 0;
        for (auto const& e : *vec) {
            visitor->array_element( elem_type, static_cast<int>( depth + 1 ), static_cast<int>( idx++ ) );
            if (elem_type->is_record())
                static_cast<const RecordInfo*>( elem_type )->visit( &e, visitor, filter, depth + 2 );
            else
                visitor->primitive( elem_type, &e );
        }
        visitor->array_end( elem_type, static_cast<int>( depth ) );
    }
};

//------------------------------------------------------------------------------

struct StringClass : public RecordInfo {
    StringClass( const char* n, TypeId i, size_t sz, size_t align ) : RecordInfo( n, i, sz, align ) {}

    void
    visit( void const* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept override
    {
        ( void ) filter;
        if (!instance) {
            visitor->string( this, nullptr );
            return;
        }
        auto* str  = static_cast<const std::string*>( instance );
        auto* cstr = str->c_str();
        visitor->string( this, &cstr );
    }
};

} // namespace ncore::rfl

//------------------------------------------------------------------------------

namespace std {
template<>
struct hash<ncore::rfl::TypeId> {
    size_t operator()( ncore::rfl::TypeId id ) const noexcept
    {
        return id.value;
    }
};
} // namespace std

//------------------------------------------------------------------------------

// TODO: may be better to use attributes after all

#define NC_FIELD_IMPL( T, m, flg, q )                                                                                  \
    ::ncore::rfl::FieldInfo{                                                                                           \
        #m,                                                                                                            \
        const_cast<::ncore::rfl::TypeInfo*>( ::ncore::rfl::Registry::find<decltype( ( ( T* ) 0 )->m )>() ),            \
        sizeof( ( ( T* ) 0 )->m ),                                                                                     \
        offsetof( T, m ),                                                                                              \
        flg,                                                                                                           \
        ::ncore::rfl::detail::category_of<decltype( ( ( T* ) 0 )->m )>(),                                              \
        q,                                                                                                             \
    },

#define NC_F( T, m )                                                                                                   \
    NC_FIELD_IMPL(                                                                                                     \
        T, m, ( ::ncore::rfl::PropertyFlags::Serializable | ::ncore::rfl::PropertyFlags::Editable ),                   \
        ::ncore::rfl::Qualifier{}                                                                                      \
    )

#define NC_FR( T, m )                                                                                                  \
    NC_FIELD_IMPL(                                                                                                     \
        T, m,                                                                                                          \
        ( ::ncore::rfl::PropertyFlags::Serializable | ::ncore::rfl::PropertyFlags::Editable |                          \
          ::ncore::rfl::PropertyFlags::ReadOnly ),                                                                     \
        ::ncore::rfl::Qualifier{}                                                                                      \
    )

#define NC_FH( T, m ) NC_FIELD_IMPL( T, m, ::ncore::rfl::PropertyFlags::Serializable, ::ncore::rfl::Qualifier{} )

//------------------------------------------------------------------------------

#define NSTRUCT( T, ... )                                                                                              \
    static ::ncore::rfl::RecordInfo& _nc_info_##T()                                                                    \
    {                                                                                                                  \
        static ::ncore::rfl::FieldInfo _nc_flds_##T[] = { __VA_ARGS__ };                                               \
        static ::ncore::rfl::RecordInfo& ci           = []() -> ::ncore::rfl::RecordInfo& {                            \
            auto& c        = ::ncore::rfl::Registry::emplace<::ncore::rfl::RecordInfo, T>( #T );                       \
            c.fields_begin = _nc_flds_##T;                                                                             \
            c.fields_end   = _nc_flds_##T + ( sizeof( _nc_flds_##T ) / sizeof( ::ncore::rfl::FieldInfo ) );            \
            return c;                                                                                                  \
        }();                                                                                                           \
        return ci;                                                                                                     \
    }                                                                                                                  \
    inline static const int _nc_trig_##T = ( _nc_info_##T(), 0 );

//------------------------------------------------------------------------------

#define NENUM_ELEMENT( EnumT, element )                                                                                \
    ::ncore::rfl::EnumElement                                                                                          \
    {                                                                                                                  \
        #element, static_cast<int64_t>( EnumT::element )                                                               \
    }

#define NENUM( T, ... )                                                                                                \
    static ::ncore::rfl::EnumInfo& _nc_enum_info_##T()                                                                 \
    {                                                                                                                  \
        static ::ncore::rfl::EnumElement _nc_enum_elems_##T[] = { __VA_ARGS__ };                                       \
        static ::ncore::rfl::EnumInfo& ei                     = []() -> ::ncore::rfl::EnumInfo& {                      \
            auto& e          = ::ncore::rfl::Registry::emplace<::ncore::rfl::EnumInfo, T>( #T );                       \
            e.elements_begin = _nc_enum_elems_##T;                                                                     \
            e.elements_end =                                                                                           \
                _nc_enum_elems_##T + ( sizeof( _nc_enum_elems_##T ) / sizeof( ::ncore::rfl::EnumElement ) );           \
            return e;                                                                                                  \
        }();                                                                                                           \
        return ei;                                                                                                     \
    }                                                                                                                  \
    inline static const int _nc_trig_enum_##T = ( _nc_enum_info_##T(), 0 );
