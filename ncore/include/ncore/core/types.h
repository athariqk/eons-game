// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: umbrella file for NCORE's params system

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

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

    bool operator==( TypeId o ) const noexcept
    {
        return value == o.value;
    }
    bool operator!=( TypeId o ) const noexcept
    {
        return value != o.value;
    }
    bool valid() const noexcept
    {
        return value != 0;
    }
    static constexpr TypeId null() noexcept
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

// Extracting type name from compiler-dependent compile-time information
// Source: https://rodusek.com/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/

template<std::size_t... Idxs>
constexpr auto substring_as_array( std::string_view str, std::index_sequence<Idxs...> )
{
    return std::array{ str[Idxs]..., '\n' };
}

template<typename T>
constexpr auto type_name_array()
{
#if defined( __clang__ )
    constexpr auto prefix   = std::string_view{ "[T = " };
    constexpr auto suffix   = std::string_view{ "]" };
    constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined( __GNUC__ )
    constexpr auto prefix   = std::string_view{ "with T = " };
    constexpr auto suffix   = std::string_view{ "]" };
    constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined( _MSC_VER )
    constexpr auto prefix   = std::string_view{ "type_name_array<" };
    constexpr auto suffix   = std::string_view{ ">(void)" };
    constexpr auto function = std::string_view{ __FUNCSIG__ };
#else
#error Unsupported compiler
#endif

    constexpr auto start = function.find( prefix ) + prefix.size();
    constexpr auto end   = function.rfind( suffix );

    static_assert( start < end );

    constexpr auto raw = function.substr( start, ( end - start ) );

    // MSVC __FUNCSIG__ prepends "class " or "struct " to user types.
    // Strip it so type_id<T>() hashes the same string reflection_gen hashes.
    constexpr bool has_class  = raw.size() > 6 && raw.substr( 0, 6 ) == "class ";
    constexpr bool has_struct = raw.size() > 7 && raw.substr( 0, 7 ) == "struct ";
    constexpr auto name       = has_class ? raw.substr( 6 ) : has_struct ? raw.substr( 7 ) : raw;

    return substring_as_array( name, std::make_index_sequence<name.size()>{} );
}

template<typename T>
struct type_name_holder {
    static inline constexpr auto value = type_name_array<T>();
};

template<typename T>
constexpr std::string_view type_name()
{
    constexpr auto& value = type_name_holder<T>::value;
    return std::string_view{ value.data(), value.size() };
}

template<typename T>
constexpr size_t type_hash() noexcept
{
    constexpr auto name = type_name<T>();
    return fnv1a( name.data(), name.size() - 1 ); // exclude trailing '\n'
}

template<typename T>
constexpr TypeId type_id() noexcept
{
    return TypeId{ detail::type_hash<T>() };
}

constexpr TypeId type_id_from_name( StringView name ) noexcept
{
    size_t h = fnv1a( name.data(), name.size() );
    return TypeId{ h ? h : 1 };
}

} // namespace detail

//------------------------------------------------------------------------------

enum class PropertyFlags : uint16_t {
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
enum class TypeKind : uint8_t {
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
    TypeKind kind  = TypeKind::INVALID;
    TypeInfo* next = nullptr; // linked list, ptr to the next chain.

    TypeInfo() : name( nullptr ), id( TypeId::null() ), size( 0 ), alignment( 0 ) {}
    TypeInfo( const char* n, TypeId i, size_t sz, size_t align ) : name( n ), id( i ), size( sz ), alignment( align ) {}

    virtual ~TypeInfo() = default;

    TypeInfo( const TypeInfo& )            = delete;
    TypeInfo& operator=( const TypeInfo& ) = delete;

    /**
     * @brief Construct an instance at the given memory location.
     * @param instance The memory location to construct the instance on.
     * @param data If non-null, copy-construct from it; otherwise zero-initialize.
     */
    virtual void construct( void* instance, const void* data = nullptr ) const;

    /**
     * @brief Destroy the instance (calls destructors for non-trivial types).
     */
    virtual void destruct( void* instance ) const;

    /**
     * @brief Copy-construct from src into dst.
     */
    virtual void clone( const void* src, void* dst ) const;

    /**
     * @brief Copy-assign from src into dst (dst must already be constructed).
     */
    virtual void replace( const void* src, void* dst ) const;

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
    bool is_unsigned                  = false;

    EnumInfo() = default;
    EnumInfo( const char* type_name, TypeId t_id, size_t type_size, size_t align ) :
        TypeInfo( type_name, t_id, type_size, align )
    {
        kind = TypeKind::ENUM;
    }

    Span<const EnumElement> elements() const noexcept
    {
        return { elements_begin, elements_end };
    }

    bool try_get_value( StringView enum_name, int64_t& out_value ) const noexcept
    {
        for (const auto& elem : elements()) {
            if (elem.name == enum_name) {
                out_value = elem.value;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Reads the enum value at instance, honoring the enum's storage width.
     */
    int64_t get_value( const void* instance ) const noexcept;

    /**
     * @brief Writes the enum value at instance, honoring the enum's storage width.
     */
    void set_value( void* instance, int64_t value ) const noexcept;

    /**
     * @brief Return the name of an enum value as static string view.
     */
    StringView get_name( int64_t value ) const noexcept
    {
        for (const auto& elem : elements()) {
            if (elem.value == value)
                return elem.name;
        }
        return "<unknown_enum_value>";
    }

    /**
     * @brief Return the name of an enum as static string view.
     */
    StringView get_name( const void* instance ) const noexcept
    {
        return get_name( get_value( instance ) );
    }

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
    RecordInfo( const char* type_name, TypeId t_id, size_t type_size, size_t align ) :
        TypeInfo( type_name, t_id, type_size, align )
    {
        kind = TypeKind::RECORD;
    }

    size_t field_count() const noexcept
    {
        return fields().size();
    }

    Span<const FieldInfo> fields() const noexcept
    {
        return { fields_begin, fields_end };
    }

    const FieldInfo* find_field( StringView n ) const noexcept;

    virtual void visit(
        void* instance, RecordVisitor* visitor, PropertyFlags filter = static_cast<PropertyFlags>( 0xFFFF ),
        unsigned depth = 0
    ) const noexcept;

    virtual void visit_field(
        void* ptr, const FieldInfo* field, RecordVisitor* visitor, PropertyFlags filter, int depth, int array_elem = -1
    ) const noexcept;

    virtual void visit_array(
        void* ptr, const FieldInfo* field, RecordVisitor* visitor, PropertyFlags filter, unsigned depth
    ) const noexcept;

    /**
     * @brief Resize a container instance to @p length elements.
     *
     * Called by RecordVisitor implementations that read serialized arrays back
     * from a stream (the count lives in the stream, not in the default-
     * constructed object). Base impl is a no-op: fixed-size arrays and
     * non-container records need no resize. VectorClass overrides it.
     */
    virtual void resize( void* instance, size_t length ) const noexcept;

    void to_string( String& out, const void* instance ) const override;

    void construct( void* instance, const void* data = nullptr ) const override;
    void destruct( void* instance ) const override;
    void clone( const void* src, void* dst ) const override;
    void replace( const void* src, void* dst ) const override;
};

/**
 * @brief RecordInfo with whole-object lifecycle for T.
 *
 * Construct/destruct/clone/replace operate on the C++ object as a whole
 * (placement-new / ~T() / copy-ctor / copy-assign) instead of walking NPROPS
 * fields. Required so members outside NPROPS (e.g. shared_ptr listener tokens)
 * are constructed and destroyed correctly when used as flecs component hooks.
 */
template<typename T>
struct RecordInfoT : public RecordInfo {
    using RecordInfo::RecordInfo;

    void construct( void* instance, const void* data = nullptr ) const override
    {
        if constexpr (!std::is_abstract_v<T>) {
            if (data) {
                if constexpr (std::is_copy_constructible_v<T>) {
                    ::new ( instance ) T( *static_cast<const T*>( data ) );
                    return;
                }
            }
            if constexpr (std::is_default_constructible_v<T>) {
                ::new ( instance ) T();
                return;
            }
        }
        RecordInfo::construct( instance, data );
    }

    void destruct( void* instance ) const override
    {
        if constexpr (!std::is_abstract_v<T>) {
            static_cast<T*>( instance )->~T();
        } else {
            RecordInfo::destruct( instance );
        }
    }

    void clone( const void* src, void* dst ) const override
    {
        if constexpr (!std::is_abstract_v<T> && std::is_copy_constructible_v<T>) {
            ::new ( dst ) T( *static_cast<const T*>( src ) );
        } else {
            RecordInfo::clone( src, dst );
        }
    }

    void replace( const void* src, void* dst ) const override
    {
        if constexpr (!std::is_abstract_v<T> && std::is_copy_assignable_v<T>) {
            *static_cast<T*>( dst ) = *static_cast<const T*>( src );
        } else {
            RecordInfo::replace( src, dst );
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
        static TypeRegistry instance;
#pragma clang diagnostic pop
        return instance;
    }

    static void initialize();
    static void shutdown();

    //------------------------------------------------------------------------------

    /**
     * @brief Registers a TypeInfo subclass for a given type.
     * TInfo is the TypeInfo subclass to construct (e.g. RecordInfo, EnumInfo, etc).
     * TRefl is the actual type to reflect.
     *
     * @param name Pretty name of the type.
     * @param extra Arguments forwarded to the TypeInfo class/subclass constructor.
     */
    template<std::derived_from<TypeInfo> TInfo, typename TRefl, typename... TArgs>
    static TInfo& register_type( const char* name, TArgs&&... extra ) noexcept
    {
        static TInfo& info = [&]() -> TInfo& {
            static TInfo instance(
                name, detail::type_id<TRefl>(), sizeof( TRefl ), alignof( TRefl ), std::forward<TArgs>( extra )...
            );
            instance.next  = type_list_head;
            type_list_head = &instance;
            // Seed type_cache so find<T>() resolves without a list walk.
            // Always overwrite: NSTRUCT_V/NCLASS/ECS_COMPONENT RecordInfoT must
            // win over a plain gen RecordInfo registered under the same TypeId.
            auto& cache = get_instance().type_cache;
            auto it     = cache.find( instance.id );
            if (it != cache.end()) {
                // Steal fields from a gen-emitted plain RecordInfo (same TypeId)
                // so the winner keeps Inspector metadata when static-init order
                // registers gen first.
                auto* old_rec = dynamic_cast<RecordInfo*>( it->second );
                auto* new_rec = dynamic_cast<RecordInfo*>( &instance );
                if (old_rec && new_rec && !new_rec->fields_begin && old_rec->fields_begin) {
                    new_rec->fields_begin = old_rec->fields_begin;
                    new_rec->fields_end   = old_rec->fields_end;
                }
            }
            apply_pending_fields_( instance );
            cache[instance.id] = &instance;
            return instance;
        }();
        return info;
    }

    /**
     * @brief Registers a plain TypeInfo for primitives/fundamentals.
     *
     * Uses the user-provided name for hashing (not __PRETTY_FUNCTION__) so
     * that the hash matches what reflection_gen produces for the same name.
     *
     * @param name The name to register the primitive type under (e.g. "int", "float", etc).
     */
    template<typename T>
    static TypeInfo& register_type( const char* name ) noexcept
    {
        static TypeInfo& info = [&]() -> TypeInfo& {
            static TTypeInfo<T> instance( name, detail::type_id_from_name( name ), sizeof( T ), alignof( T ) );
            instance.next                          = type_list_head;
            type_list_head                         = &instance;
            get_instance().type_cache[instance.id] = &instance;
            return instance;
        }();
        return info;
    }

    /**
     * @brief Registers a pre-constructed TypeInfo instance.
     *
     * The caller owns the lifetime of the TypeInfo object (typically a
     * static-local). This overload is used by code-generated reflection
     * that doesn't have access to the reflected type T.
     */
    static void register_type( TypeInfo* p_info ) noexcept;

    /**
     * @brief Attach (or defer) field metadata for a TypeId.
     *
     * Used by reflection_gen for NC_COMPONENT types so fields land on the
     * winning RecordInfoT regardless of static-init order. If the type is not
     * registered yet, fields are stashed and applied in initialize() (and when
     * register_type<TInfo,TRefl> later creates the RecordInfoT).
     *
     * Never overwrites non-empty fields (NPROPS / earlier provide_fields win).
     */
    static void provide_fields( TypeId id, const FieldInfo* begin, const FieldInfo* end ) noexcept;

    // TODO: add register_class<T>() helper method

    //------------------------------------------------------------------------------

    static const TypeInfo* find( TypeId id ) noexcept
    {
        if (!id.valid())
            return nullptr;

        auto& map = get_instance().type_cache;
        auto it   = map.find( id );
        if (it != map.end()) {
            rtti_hits_++;
            return it->second;
        }

        for (auto* c = type_list_head; c; c = c->next) {
            rtti_hits_++;
            if (c->id == id) {
                map[id] = c;
                NC_LOG_DEBUG( "TypeRegistry: cached type name='{}' with ID={}", c->name, id.value );
                return c;
            }
        }

        NC_LOG_WARN( "TypeRegistry: type ID={} not found, has it been reflected?", id.value );
        return nullptr;
    }

    static const TypeInfo* find( StringView name ) noexcept
    {
        for (auto* c = type_list_head; c; c = c->next) {
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

    static const TypeInfo& get( TypeId id ) noexcept;
    static const TypeInfo& get( StringView name ) noexcept;

    static const StringView get_type_name( TypeId id ) noexcept
    {
        auto* c = find( id );
        return c ? c->name : "<unknown>";
    }

    /**
     * @brief Check if a type has been registered to TypeRegistry.
     */
    static bool contains( StringView name ) noexcept
    {
        for (auto* c = type_list_head; c; c = c->next) {
            rtti_hits_++;
            if (std::strcmp( name.data(), c->name ) == 0)
                return true;
        }
        return false;
    }

    static void to_string( String& out, const void* instance, TypeId id ) noexcept
    {
        auto t = find( id );
        if (!t)
            out = "UnknownType";
        return t->to_string( out, instance );
    }

    //------------------------------------------------------------------------------

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
    static bool contains() noexcept
    {
        TypeId id = detail::type_id<T>();
        for (auto* c = type_list_head; c; c = c->next) {
            rtti_hits_++;
            if (id == c->id)
                return true;
        }
        return false;
    }

    /**
     * @brief Hard exits if we can't find the type info.
     */
    template<typename T>
    static const TypeInfo& get() noexcept
    {
        NC_ASSERT_MSG(
            contains<T>(),
            std::format( "Type '{}' is not found in the registry", detail::type_name<T>().data() ).c_str()
        );
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
        NC_ASSERT_MSG(
            c, std::format( "Type '{}' is not found in the registry", detail::type_name<T>().data() ).c_str()
        );
        return *c;
    }

    template<typename T>
    static void to_string( String& out, const void* instance )
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

    static void apply_pending_fields_( TypeInfo& info ) noexcept;

    HashMap<TypeId, TypeInfo*> type_cache;
    // Deferred field ranges from provide_fields() before the TypeId is cached.
    HashMap<TypeId, std::pair<const FieldInfo*, const FieldInfo*>> pending_fields_;
};

//------------------------------------------------------------------------------

struct NCAPI RecordVisitor {
    RecordVisitor()          = default;
    virtual ~RecordVisitor() = default;

    RecordVisitor( const RecordVisitor& )            = delete;
    RecordVisitor& operator=( const RecordVisitor& ) = delete;

    virtual void class_begin( const RecordInfo* c, int depth ) = 0;
    virtual void class_end( const RecordInfo* c, int depth )   = 0;
    virtual void class_member( const FieldInfo* f, int depth ) = 0;

    /**
     * @param container_type RTTI of the container being walked (RecordInfo /
     *        VectorClass). Readers use it to call resize() before elements are
     *        visited; writers ignore it.
     * @param container Address of the container itself (the vector instance, or
     *        the inline array field for fixed arrays).
     */
    virtual void
    array_begin( const TypeInfo* t, const RecordInfo* container_type, void* container, int depth, int length ) = 0;
    virtual void array_end( const TypeInfo* t, int depth )                                                     = 0;
    virtual void array_element( const TypeInfo* t, int depth, int elem )                                       = 0;

    virtual void primitive( const TypeInfo* t, void* instance ) = 0;
    virtual void string( const TypeInfo* t, void* instance )    = 0;
};

//------------------------------------------------------------------------------

template<typename VecT>
struct VectorClass : public RecordInfo {
    VectorClass( const char* n, TypeId i, size_t sz, size_t align ) : RecordInfo( n, i, sz, align )
    {
        this->kind = TypeKind::VECTOR;
    }

    void resize( void* instance, size_t length ) const noexcept override
    {
        static_cast<VecT*>( instance )->resize( length );
    }

    void visit( void* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept override
    {
        if (!instance) {
            visitor->primitive( this, nullptr );
            return;
        }

        auto* vec       = static_cast<VecT*>( instance );
        auto* elem_type = TypeRegistry::find<typename VecT::value_type>();

        if (!elem_type) {
            // Element type was never registered (e.g. TypeRegistry::initialize()
            // not run in this process). Bail out before the element loop so the
            // archive size check fails loudly instead of dereferencing null.
            NC_LOG_ERROR( "VectorClass: element type not registered for vector '{}'", this->name );
            visitor->primitive( this, instance );
            return;
        }

        visitor->array_begin( elem_type, this, vec, static_cast<int>( depth ), static_cast<int>( vec->size() ) );
        size_t idx = 0;
        for (auto& e : *vec) {
            visitor->array_element( elem_type, static_cast<int>( depth + 1 ), static_cast<int>( idx++ ) );
            if (elem_type->is_record())
                static_cast<const RecordInfo*>( elem_type )->visit( &e, visitor, filter, depth + 2 );
            else if (elem_type->is_string())
                visitor->string( elem_type, &e );
            else
                visitor->primitive( elem_type, &e );
        }
        visitor->array_end( elem_type, static_cast<int>( depth ) );
    }
};

//------------------------------------------------------------------------------

struct NCAPI StringClass : public RecordInfo {
    StringClass( const char* n, TypeId i, size_t sz, size_t align ) : RecordInfo( n, i, sz, align )
    {
        this->kind = TypeKind::STRING;
    }

    void to_string( String& out, const void* instance ) const override;

    void construct( void* instance, const void* data = nullptr ) const override;
    void destruct( void* instance ) const override;
    void clone( const void* src, void* dst ) const override;
    void replace( const void* src, void* dst ) const override;

    void visit( void* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept override
    {
        ( void ) filter;
        visitor->string( this, instance );
    }
};

//------------------------------------------------------------------------------

template<typename T>
constexpr const char* get_enum_name( const T* value ) noexcept
{
    using RawType = std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>;
    return static_cast<const EnumInfo&>( TypeRegistry::get<RawType>() ).get_name( value ).data();
}

} // namespace nc::rtti

//------------------------------------------------------------------------------

#define NC_PROPS_BEGIN()                                                                                               \
    static const ::nc::rtti::FieldInfo* nc_get_fields_( size_t& out_count )                                            \
    {                                                                                                                  \
        static const ::nc::rtti::FieldInfo fields[] = {

#define ADD_PROPERTY_IMPL( Member, Flags, ... )                                                                        \
    []() -> ::nc::rtti::FieldInfo {                                                                                    \
        using MemberType = decltype( Self::Member );                                                                   \
        return ::nc::rtti::FieldInfo{                                                                                  \
            #Member, ::nc::rtti::detail::field_type_id<MemberType>(),  sizeof( MemberType ), offsetof( Self, Member ), \
            Flags,   ::nc::rtti::detail::field_qualifier<MemberType>()                                                 \
        };                                                                                                             \
    }(),

#define ADD_PROPERTY( ... )                                                                                            \
    ADD_PROPERTY_IMPL( __VA_ARGS__, ::nc::rtti::PropertyFlags::SERIALIZABLE | ::nc::rtti::PropertyFlags::EDITABLE )

#define NC_PROPS_END( T )                                                                                              \
    }                                                                                                                  \
    ;                                                                                                                  \
    out_count = sizeof( fields ) / sizeof( fields[0] );                                                                \
    return fields;                                                                                                     \
    }                                                                                                                  \
                                                                                                                       \
    inline static void nc_fields_init_()                                                                               \
    {                                                                                                                  \
        size_t count = 0;                                                                                              \
        auto* begin  = nc_get_fields_( count );                                                                        \
        ::nc::rtti::TypeRegistry::provide_fields( nc_info_().id, begin, begin + count );                               \
    }                                                                                                                  \
    inline static const int nc_register_fields_ = ( nc_fields_init_(), 0 );

//------------------------------------------------------------------------------

/**
 * @brief Registers a record type in TypeRegistry (RecordInfoT lifecycle).
 * Place inside the struct/union body.
 *
 * Prefer NC_COMPONENT / NC_COMPONENT_API for Flecs-facing types.
 * NSTRUCT_V remains for non-ECS value types (Quaternion, Color, …).
 * Use NPROPS_* macros to register fields, or REFLECT + reflection_gen.
 */
#define NSTRUCT_V( T )                                                                                                 \
    using Self = T;                                                                                                    \
    inline static ::nc::rtti::RecordInfo& nc_info_()                                                                   \
    {                                                                                                                  \
        static ::nc::rtti::RecordInfo& ci = []() -> ::nc::rtti::RecordInfo& {                                          \
            auto& c = ::nc::rtti::TypeRegistry::register_type<::nc::rtti::RecordInfoT<T>, T>( #T );                    \
            return c;                                                                                                  \
        }();                                                                                                           \
        return ci;                                                                                                     \
    }                                                                                                                  \
    inline static const int nc_register_##T = ( nc_info_(), 0 );                                                       \
    const ::nc::rtti::RecordInfo& get_class_info()                                                                     \
    {                                                                                                                  \
        return nc_info_();                                                                                             \
    }

#if __has_cpp_attribute( clang::annotate )
#define REFLECT __attribute__( ( annotate( "Reflect" ) ) )
#else
#define REFLECT
#endif

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
            e.is_unsigned    = std::is_unsigned_v<std::underlying_type_t<T>>;                                           \
            return e;                                                                                                   \
        }();                                                                                                            \
        return ei;                                                                                                      \
    }                                                                                                                   \
    inline static const int nc_trig_enum_##T = ( nc_enum_info_##T(), 0 );

//------------------------------------------------------------------------------

#define NENUM_GET_NAME( p ) ::nc::rtti::get_enum_name( p )
