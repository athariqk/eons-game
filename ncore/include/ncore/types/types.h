// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: Core RTTI types

#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

#include <ncore.h>
#include <ncore/core/collection.h>

// C++ reflection schema inspired by Arvid Gerstmann's metareflect:
// https://github.com/Leandros/metareflect

namespace nc {

/**
 * @brief Forward declaration. The default allocator is declared once, here, so
 * unqualified spellings (Ref<T>) work in headers that only see this forward
 * declaration. The definition in object/reference.h must not repeat it.
 */
template<typename T, typename TAlloc = NcAllocator<T>>
class Ref;

} // namespace nc

namespace nc::types {

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

} // namespace nc::types

//------------------------------------------------------------------------------

namespace std {
template<>
struct hash<nc::types::TypeId> {
    size_t operator()( nc::types::TypeId id ) const noexcept
    {
        return id.value;
    }
};
} // namespace std

//------------------------------------------------------------------------------

namespace nc::types {

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
    REF // handle of reference-counted object.
};

//------------------------------------------------------------------------------

struct NCAPI Qualifier {
    uint32_t array_length = 0;
    uint8_t pointer_count = 0;
    bool is_cstring       = false;
    bool is_ref           = false;

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

template<typename T>
struct is_std_array : std::false_type {};

template<typename T, size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template<typename T>
inline constexpr bool is_std_array_v = is_std_array<std::remove_cvref_t<T>>::value;

template<typename T>
struct std_array_traits;

template<typename T, size_t N>
struct std_array_traits<std::array<T, N>> {
    using element_type             = T;
    static constexpr size_t extent = N;
};

/**
 * @brief Infers the TypeKind for a fundamental type at compile time.
 */
template<typename T>
constexpr TypeKind kind_of() noexcept
{
    using raw = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<raw, std::byte>) {
        // std::byte is technically an enum (enum class byte : unsigned char {}),
        // but we classify it as UINT8 (via this special case) so it participates
        // cleanly as a primitive for vectors, drawing, etc.
        return TypeKind::UINT8;
    } else if constexpr (std::is_same_v<raw, bool>) {
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
    } else if constexpr (is_std_array_v<raw>) {
        return type_id<typename std_array_traits<raw>::element_type>();
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
    } else if constexpr (is_std_array_v<raw>) {
        q.array_length = static_cast<uint32_t>( std_array_traits<raw>::extent );
    }
    return q;
}

} // namespace detail

//------------------------------------------------------------------------------

/**
 * @brief IValueOps defines the interface for value-semantics operations on a reflected type.
 */
struct IValueOps {
    virtual ~IValueOps() = default;
    /**
     * @brief Construct an instance at the given memory location.
     * @param instance The memory location to construct the instance on.
     * @param data If non-null, copy-construct from it; otherwise zero-initialize.
     */
    virtual void construct( void* instance, const void* data = nullptr ) const = 0;
    /**
     * @brief Destroy the instance (calls destructors for non-trivial types).
     */
    virtual void destruct( void* instance ) const = 0;
    /**
     * @brief Copy-construct from src into dst.
     */
    virtual void clone( const void* src, void* dst ) const = 0;
    /**
     * @brief Copy-assign from src into dst (dst must already be constructed).
     */
    virtual void replace( const void* src, void* dst ) const = 0;
};

} // namespace nc::types
