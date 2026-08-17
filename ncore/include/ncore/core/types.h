// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: umbrella file for NCORE's params system

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

#include <ncore.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

#include "collection.h"

// Core params types.
// Inspired by Arvid Gerstmann's metareflect
// https://github.com/Leandros/metareflect

namespace nc::rtti {

//------------------------------------------------------------------------------

struct NCAPI TypeId {
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

} // namespace nc::rtti

//------------------------------------------------------------------------------

namespace std {
template<>
struct hash<nc::rtti::TypeId> {
    size_t operator()( nc::rtti::TypeId id ) const noexcept
    {
        return id.value;
    }
};
} // namespace std

//------------------------------------------------------------------------------

namespace nc::rtti {

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
    constexpr StringView sig = __PRETTY_FUNCTION__;
#elif defined( _MSC_VER )
    constexpr StringView sig = __FUNCSIG__;
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

enum class NCAPI PropertyFlags : uint16_t {
    NONE         = 0,
    SERIALIZABLE = 1 << 0,
    EDITABLE     = 1 << 1,
    READ_ONLY    = 1 << 2,
    HIDDEN       = 1 << 3,
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
    return ( f & check ) != PropertyFlags::NONE;
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

/**
 * @brief The kind of a reflected type.
 *
 * Populated once at registration (primitive kinds via detail::kind_of<T>(),
 * records/enums/strings/vectors via their TypeInfo subclass constructors).
 * This is the single source of truth for type category — fields derive their
 * category from `field.get_type()->kind` plus the field qualifier.
 */
enum class NCAPI TypeKind : uint8_t {
    INVALID,
    BOOL,
    INT8,
    UINT8,
    INT16,
    UINT16,
    INT32,
    UINT32,
    INT64,
    UINT64,
    FLOAT,
    DOUBLE,
    STRING,
    POINTER,
    ENUM,
    RECORD,
    VECTOR,
};

//------------------------------------------------------------------------------

struct NCAPI Qualifier {
    uint32_t array_length = 0;
    uint8_t pointer_count = 0;
    bool is_cstring       = false;

    bool is_array() const noexcept
    {
        return array_length > 0;
    }
    bool is_pointer() const noexcept
    {
        return pointer_count > 0;
    }
};

//------------------------------------------------------------------------------

namespace detail {

/**
 * @brief Infers the TypeKind for a fundamental type at compile time.
 */
template<typename T>
constexpr TypeKind kind_of() noexcept
{
    using raw = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<raw, bool>) {
        return TypeKind::BOOL;
    } else if constexpr (std::is_floating_point_v<raw>) {
        if constexpr (sizeof( raw ) == 4)
            return TypeKind::FLOAT;
        else if constexpr (sizeof( raw ) == 8)
            return TypeKind::DOUBLE;
        else
            return TypeKind::INVALID;
    } else if constexpr (std::is_integral_v<raw>) {
        if constexpr (std::is_signed_v<raw>) {
            if constexpr (sizeof( raw ) == 1)
                return TypeKind::INT8;
            else if constexpr (sizeof( raw ) == 2)
                return TypeKind::INT16;
            else if constexpr (sizeof( raw ) == 4)
                return TypeKind::INT32;
            else if constexpr (sizeof( raw ) == 8)
                return TypeKind::INT64;
            else
                return TypeKind::INVALID;
        } else {
            if constexpr (sizeof( raw ) == 1)
                return TypeKind::UINT8;
            else if constexpr (sizeof( raw ) == 2)
                return TypeKind::UINT16;
            else if constexpr (sizeof( raw ) == 4)
                return TypeKind::UINT32;
            else if constexpr (sizeof( raw ) == 8)
                return TypeKind::UINT64;
            else
                return TypeKind::INVALID;
        }
    } else if constexpr (std::is_pointer_v<raw>) {
        return TypeKind::POINTER;
    } else if constexpr (std::is_enum_v<raw>) {
        return TypeKind::ENUM;
    } else {
        return TypeKind::INVALID;
    }
}

/**
 * @brief Decomposes a field type into its element type id.
 *
 * Pointer and array fields are stripped: the FieldInfo stores the pointee /
 * element type id, while the field's Qualifier records pointer/array-ness.
 */
template<typename F>
constexpr TypeId field_type_id() noexcept
{
    using raw = std::remove_cvref_t<F>;
    if constexpr (std::is_pointer_v<raw>) {
        return type_id<std::remove_pointer_t<raw>>();
    } else if constexpr (std::is_array_v<raw>) {
        return type_id<std::remove_extent_t<raw>>();
    } else {
        return type_id<F>();
    }
}

/**
 * @brief Decomposes a field type into its Qualifier.
 */
template<typename F>
constexpr Qualifier field_qualifier() noexcept
{
    using raw = std::remove_cvref_t<F>;
    Qualifier q;
    if constexpr (std::is_pointer_v<raw>) {
        q.pointer_count = 1;
        q.is_cstring    = std::is_same_v<std::remove_cv_t<std::remove_pointer_t<raw>>, char>;
    } else if constexpr (std::is_array_v<raw>) {
        q.array_length = static_cast<uint32_t>( std::extent_v<raw> );
    }
    return q;
}

} // namespace detail

//------------------------------------------------------------------------------

struct NCAPI TypeInfo {
    const char* name;
    TypeId id;
    size_t size;
    size_t alignment;
    TypeKind kind   = TypeKind::INVALID;
    TypeInfo* _next = nullptr;

    TypeInfo() : name( nullptr ), id( TypeId::null() ), size( 0 ), alignment( 0 ) {}
    TypeInfo( const char* n, TypeId i, size_t sz, size_t align ) : name( n ), id( i ), size( sz ), alignment( align ) {}

    virtual ~TypeInfo() = default;

    /**
     * @brief Returns true if this type is a composite data structure (class, structs, etc).
     */
    bool is_record() const noexcept
    {
        return kind == TypeKind::RECORD || kind == TypeKind::VECTOR;
    }

    bool is_primitive() const noexcept
    {
        return kind >= TypeKind::BOOL && kind <= TypeKind::DOUBLE;
    }

    bool is_integral() const noexcept
    {
        return kind >= TypeKind::BOOL && kind <= TypeKind::UINT64;
    }

    bool is_floating() const noexcept
    {
        return kind == TypeKind::FLOAT || kind == TypeKind::DOUBLE;
    }

    bool is_string() const noexcept
    {
        return kind == TypeKind::STRING;
    }

    bool is_enum() const noexcept
    {
        return kind == TypeKind::ENUM;
    }

    bool is_container() const noexcept
    {
        return kind == TypeKind::VECTOR;
    }

    virtual void to_string( String& out, const void* instance ) const;
};

/**
 * @brief TypeInfo specialization for fundamental types.
 *
 * Infers the TypeKind at compile time via detail::kind_of<T>().
 */
template<typename T>
struct TTypeInfo : public TypeInfo {
    TTypeInfo( const char* n, TypeId i, size_t sz, size_t align ) : TypeInfo( n, i, sz, align )
    {
        kind = detail::kind_of<T>();
    }
};

//------------------------------------------------------------------------------

struct NCAPI FieldInfo {
    StringView name;
    TypeId type_id;
    size_t width;
    size_t offset;
    PropertyFlags flags;
    Qualifier qualifier;

    const TypeInfo* get_type() const;

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
        return reinterpret_cast<T*>( static_cast<uint8_t*>( instance ) + offset );
    }

    template<typename T>
    const T* get_ptr( const void* instance ) const noexcept
    {
        return reinterpret_cast<const T*>( static_cast<const uint8_t*>( instance ) + offset );
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

    void to_string( String& out, const void* instance ) const;
};

//------------------------------------------------------------------------------

struct NCAPI EnumElement {
    StringView name;
    int64_t value;
};

/**
 * @brief EnumInfo represents a reflected enumeration type.
 */
struct NCAPI EnumInfo : public TypeInfo {
    const EnumElement* elements_begin = nullptr;
    const EnumElement* elements_end   = nullptr;

    EnumInfo() = default;
    EnumInfo( const char* name, TypeId t_id, size_t size, size_t align ) : TypeInfo( name, t_id, size, align )
    {
        kind = TypeKind::ENUM;
    }

    std::span<const EnumElement> elements() const noexcept
    {
        return { elements_begin, elements_end };
    }

    bool try_get_value( StringView name, int64_t& out_value ) const noexcept
    {
        for (const auto& elem : elements()) {
            if (elem.name == name) {
                out_value = elem.value;
                return true;
            }
        }
        return false;
    }

    StringView get_name( int64_t value ) const noexcept
    {
        for (const auto& elem : elements()) {
            if (elem.value == value)
                return elem.name;
        }
        return "<unknown_enum_value>";
    }

    /**
     * @brief Reads the enum value at instance, honoring the enum's storage width.
     */
    int64_t get_value( const void* instance ) const noexcept;

    /**
     * @brief Writes the enum value at instance, honoring the enum's storage width.
     */
    void set_value( void* instance, int64_t value ) const noexcept;

    void to_string( String& out, const void* instance ) const override;
};

//------------------------------------------------------------------------------

struct RecordVisitor;

/**
 * @brief RecordInfo represents a composite data structure (class, structs, etc).
 */
struct NCAPI RecordInfo : public TypeInfo {
    TypeId parent_id              = TypeId::null();
    const FieldInfo* fields_begin = nullptr;
    const FieldInfo* fields_end   = nullptr;

    RecordInfo() = default;
    RecordInfo( const char* name, TypeId t_id, size_t size, size_t align ) : TypeInfo( name, t_id, size, align )
    {
        kind = TypeKind::RECORD;
    }

    size_t field_count() const noexcept
    {
        return fields().size();
    }

    std::span<const FieldInfo> fields() const noexcept
    {
        return { fields_begin, fields_end };
    }

    const FieldInfo* find_field( StringView n ) const noexcept
    {
        for (auto& f : fields())
            if (f.name == n)
                return &f;
        return nullptr;
    }

    void to_string( String& out, const void* instance ) const override;

    virtual void visit(
        const void* instance, RecordVisitor* visitor, PropertyFlags filter = static_cast<PropertyFlags>( 0xFFFF ),
        unsigned depth = 0
    ) const noexcept;

    virtual void visit_field(
        const void* ptr, const FieldInfo* field, RecordVisitor* visitor, PropertyFlags filter, int depth,
        int array_elem = -1
    ) const noexcept;

    virtual void visit_array(
        const void* ptr, const FieldInfo* field, RecordVisitor* visitor, PropertyFlags filter, unsigned depth
    ) const noexcept;

    /**
     * @brief Construct the instance at the pointed location.
     * @param instance Pointer to instance to construct.
     * @param value If not NULL, this will do copy-construction.
     */
    virtual void construct( void* instance, const void* data = nullptr ) const = 0;
    virtual void destruct( void* instance ) const                              = 0;
    virtual void clone( const void* src, void* dst ) const                     = 0;
    virtual void replace( const void* src, void* dst ) const                   = 0;
};

template<typename T>
struct TRecordInfo : public RecordInfo {
    TRecordInfo( const char* name, TypeId t_id, size_t size, size_t align ) : RecordInfo( name, t_id, size, align ) {}

    void construct( void* instance, const void* data = nullptr ) const override
    {
        if constexpr (std::is_abstract_v<T>)
            return;

        if (data) {
            if constexpr (std::is_copy_constructible_v<T>) {
                new ( instance ) T( *static_cast<const T*>( data ) );
            }
        } else {
            if constexpr (std::is_default_constructible_v<T>) {
                new ( instance ) T();
            }
        }
    }

    void destruct( void* instance ) const override
    {
        if constexpr (std::is_destructible_v<T>)
            static_cast<T*>( instance )->~T();
    }

    void clone( const void* src, void* dst ) const override
    {
        if constexpr (std::is_copy_assignable_v<T>)
            *static_cast<T*>( dst ) = *static_cast<const T*>( src );
    }

    void replace( const void* src, void* dst ) const override
    {
        const T* src_obj = static_cast<const T*>( src );
        T* dst_obj       = static_cast<T*>( dst );

        if constexpr (std::is_move_assignable_v<T> && !std::is_const_v<std::remove_reference_t<decltype( *src_obj )>>) {
            *dst_obj = std::move( *src_obj );
        } else if constexpr (std::is_copy_assignable_v<T>) {
            clone( src, dst );
        } else {
            // static_assert( false, "Component T must be copy or move assignable" );
        }
    }
};

//------------------------------------------------------------------------------

/**
 * @brief A global registry of reflected types and classes.
 * Equivalent to Godot's ClassDB.
 */
class NCAPI TypeRegistry {
public:
    TypeRegistry( const TypeRegistry& )            = delete;
    TypeRegistry& operator=( const TypeRegistry& ) = delete;

    TypeRegistry( TypeRegistry&& )            = delete;
    TypeRegistry& operator=( TypeRegistry&& ) = delete;

    static TypeRegistry& get_instance()
    {
        static TypeRegistry instance;
        return instance;
    }

    static void initialize();
    static void shutdown();

    /**
     * @brief Registers a TypeInfo subclass for a given type T.
     *
     * @param TI The TypeInfo subclass to construct (e.g. RecordInfo, EnumInfo, etc).
     * @param T The actual type to reflect.
     * @param TArgs Arguments forwarded to the TypeInfo class/subclass constructor.
     */
    template<std::derived_from<TypeInfo> TI, typename T, typename... TArgs>
    static TI& register_type( const char* name, TArgs&&... extra ) noexcept
    {
        static TI info( name, detail::type_id<T>(), sizeof( T ), alignof( T ), std::forward<TArgs>( extra )... );
        static const bool registered = [] {
            info._next     = type_list_head;
            type_list_head = &info;
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
    static TypeInfo& register_type( const char* name ) noexcept
    {
        return register_type<TTypeInfo<T>, T>( name );
    }

    // TODO: add register_class<T>() helper method

    static const TypeInfo* find( TypeId id ) noexcept
    {
        auto& map = get_instance().type_cache;
        auto it   = map.find( id );
        if (it != map.end()) {
            rtti_hits_++;
            return it->second;
        }

        for (auto* c = type_list_head; c; c = c->_next) {
            rtti_hits_++;
            if (c->id == id) {
                map[id] = c;
                NC_LOG_DEBUG( "TypeRegistry: cached type={} with ID={}", c->name, id.value );
                return c;
            }
        }

        NC_LOG_WARN( "TypeRegistry: type ID={} not found, has it been reflected?", id.value );
        return nullptr;
    }

    static const TypeInfo* find( StringView name ) noexcept
    {
        for (auto* c = type_list_head; c; c = c->_next) {
            rtti_hits_++;
            if (name == c->name)
                return c;
        }

        NC_LOG_WARN( "TypeRegistry: type name={} not found, has it been reflected?", name );
        return nullptr;
    }

    static const RecordInfo* find_record( TypeId id ) noexcept
    {
        const TypeInfo* t = find( id );
        if (!t)
            return nullptr;
        if (!t->is_record()) {
            NC_LOG_WARN( "Regsitry: type name={} is found but is not a record type", t->name );
            return nullptr;
        }
        return static_cast<const RecordInfo*>( t );
    }

    static const void to_string( String& out, void* instance, TypeId id ) noexcept
    {
        auto t = find( id );
        if (!t)
            out = "UnknownType";
        return t->to_string( out, instance );
    }

    static const TypeInfo& get( TypeId id ) noexcept;
    static const TypeInfo& get( StringView name ) noexcept;

    static const StringView get_type_name( TypeId id ) noexcept
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
        NC_ASSERT( is_registered<T>(), "Type is not found in the registry" );
        return get( detail::type_id<T>() );
    }

    /**
     * @return The hashed id of the type.
     */
    template<typename T>
    static const TypeId get_type_id() noexcept
    {
        return detail::type_id<T>();
    }

    template<typename T>
    static const StringView get_type_name()
    {
        return get_type_name( detail::type_id<T>() );
    }

    // hard-exits version, mirroring get<T>()
    template<typename T>
    static const RecordInfo& get_record() noexcept
    {
        const RecordInfo* c = find_record<T>();
        NC_ASSERT( c, "Record type is not found in the registry" );
        return *c;
    }

    template<typename T>
    static const void to_string( String& out, void* instance )
    {
        return to_string( out, instance, detail::type_id<T>() );
    }

    static int get_rtti_hits()
    {
        return rtti_hits_;
    }

private:
    TypeRegistry();

    static TypeInfo* type_list_head;
    static int rtti_hits_;

    HashMap<TypeId, TypeInfo*> type_cache;
};

//------------------------------------------------------------------------------

struct NCAPI RecordVisitor {
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
struct VectorClass : public TRecordInfo<VecT> {
    VectorClass( const char* n, TypeId i, size_t sz, size_t align ) : TRecordInfo<VecT>( n, i, sz, align )
    {
        this->kind = TypeKind::VECTOR;
    }

    void
    visit( void const* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept override
    {
        if (!instance) {
            visitor->primitive( this, nullptr );
            return;
        }

        auto* vec       = static_cast<const VecT*>( instance );
        auto* elem_type = TypeRegistry::find<typename VecT::value_type>();

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

struct NCAPI StringClass : public TRecordInfo<String> {
    StringClass( const char* n, TypeId i, size_t sz, size_t align ) : TRecordInfo( n, i, sz, align )
    {
        this->kind = TypeKind::STRING;
    }

    void to_string( String& out, const void* instance ) const override;

    void
    visit( void const* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept override
    {
        ( void ) filter;
        if (!instance) {
            visitor->string( this, nullptr );
            return;
        }
        auto* str  = static_cast<const String*>( instance );
        auto* cstr = str->c_str();
        visitor->string( this, &cstr );
    }
};

} // namespace nc::rtti

//------------------------------------------------------------------------------

// TODO: may be better to use attributes after all

#define NC_FIELD_IMPL( T, m, flg )                                                                                     \
    ::nc::rtti::FieldInfo                                                                                              \
    {                                                                                                                  \
        #m, ::nc::rtti::detail::field_type_id<decltype( ( ( T* ) 0 )->m )>(), sizeof( ( ( T* ) 0 )->m ),               \
            offsetof( T, m ), flg, ::nc::rtti::detail::field_qualifier<decltype( ( ( T* ) 0 )->m )>(),                 \
    }

#define NC_F( T, m )                                                                                                   \
    NC_FIELD_IMPL( T, m, ( ::nc::rtti::PropertyFlags::SERIALIZABLE | ::nc::rtti::PropertyFlags::EDITABLE ) )

#define NC_FR( T, m )                                                                                                  \
    NC_FIELD_IMPL(                                                                                                     \
        T, m,                                                                                                          \
        ( ::nc::rtti::PropertyFlags::SERIALIZABLE | ::nc::rtti::PropertyFlags::EDITABLE |                              \
          ::nc::rtti::PropertyFlags::READ_ONLY )                                                                       \
    )

#define NC_FH( T, m ) NC_FIELD_IMPL( T, m, ::nc::rtti::PropertyFlags::SERIALIZABLE )

//------------------------------------------------------------------------------

#define NSTRUCT1( T )                                                                                                  \
    inline static ::nc::rtti::TRecordInfo<T>& nc_info_##T()                                                            \
    {                                                                                                                  \
        static ::nc::rtti::TRecordInfo<T>& ci = []() -> ::nc::rtti::TRecordInfo<T>& {                                  \
            auto& c = ::nc::rtti::TypeRegistry::register_type<::nc::rtti::TRecordInfo<T>, T>( #T );                    \
            return c;                                                                                                  \
        }();                                                                                                           \
        return ci;                                                                                                     \
    }                                                                                                                  \
    inline static const int nc_trig_##T = ( nc_info_##T(), 0 );

#define NSTRUCTV( T, ... )                                                                                             \
    inline static ::nc::rtti::TRecordInfo<T>& nc_info_##T()                                                            \
    {                                                                                                                  \
        static ::nc::rtti::FieldInfo nc_flds_##T[] = { __VA_ARGS__ };                                                  \
        static ::nc::rtti::TRecordInfo<T>& ci      = []() -> ::nc::rtti::TRecordInfo<T>& {                             \
            auto& c        = ::nc::rtti::TypeRegistry::register_type<::nc::rtti::TRecordInfo<T>, T>( #T );             \
            c.fields_begin = nc_flds_##T;                                                                              \
            c.fields_end   = nc_flds_##T + ( sizeof( nc_flds_##T ) / sizeof( ::nc::rtti::FieldInfo ) );                \
            return c;                                                                                                  \
        }();                                                                                                           \
        return ci;                                                                                                     \
    }                                                                                                                  \
    inline static const int nc_trig_##T = ( nc_info_##T(), 0 );

//------------------------------------------------------------------------------

#define NENUM_ELEMENT( EnumT, element )                                                                                \
    ::nc::rtti::EnumElement                                                                                            \
    {                                                                                                                  \
        #element, static_cast<int64_t>( EnumT::element )                                                               \
    }

#define NENUM( T, ... )                                                                                                 \
    inline static ::nc::rtti::EnumInfo& nc_enum_info_##T()                                                              \
    {                                                                                                                   \
        static ::nc::rtti::EnumElement nc_enum_elems_##T[] = { __VA_ARGS__ };                                           \
        static ::nc::rtti::EnumInfo& ei                    = []() -> ::nc::rtti::EnumInfo& {                            \
            auto& e          = ::nc::rtti::TypeRegistry::register_type<::nc::rtti::EnumInfo, T>( #T );                  \
            e.elements_begin = nc_enum_elems_##T;                                                                       \
            e.elements_end   = nc_enum_elems_##T + ( sizeof( nc_enum_elems_##T ) / sizeof( ::nc::rtti::EnumElement ) ); \
            return e;                                                                                                   \
        }();                                                                                                            \
        return ei;                                                                                                      \
    }                                                                                                                   \
    inline static const int nc_trig_enum_##T = ( nc_enum_info_##T(), 0 );
