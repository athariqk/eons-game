// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: variant.h - boxing of arbitrary types.

#pragma once

#include <algorithm>
#include <limits>

#include <ncore/object/object_info.h>

#include "classinfo.h"
#include "typedb.h"
#include "typeinfo.h"

namespace nc {

// ---------------------------------------------------------------------------

template<bool IsConst>
class BasicTypedPtr {
public:
    using Pointer = std::conditional_t<IsConst, const void*, void*>;

    BasicTypedPtr() = default;

    BasicTypedPtr( const types::TypeInfo* p_type, Pointer p_ptr ) noexcept : type( p_type ), ptr( p_ptr ) {}

    // Allow implicit conversion from non-const TypedPtr to ConstTypedPtr
    template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
    BasicTypedPtr( const BasicTypedPtr<OtherConst>& other ) noexcept : type( other.get_type() ), ptr( other.get_ptr() )
    {}

    template<typename T>
    static BasicTypedPtr create( std::conditional_t<IsConst, const T&, T&> v )
    {
        static_assert( IsConst || !std::is_const_v<T>, "TypedPtr::create requires a non-const type" );
        const types::TypeInfo* ti = TypeDB::find<T>();
        NC_ASSERT_MSG( ti, "TypedPtr type not reflected" );
        return { ti, std::addressof( v ) };
    }

    explicit operator bool() const noexcept
    {
        return ptr && type;
    }

    // -----------------------------------------------------------------

    template<typename T>
    bool is() const noexcept
    {
        return type && type->id == types::detail::type_id<T>();
    }

    bool is_a( types::TypeId id ) const noexcept
    {
        for (const types::TypeInfo* t = type; t;) {
            if (t->id == id)
                return true;
            if (!t->is_record())
                return false;
            t = TypeDB::find( static_cast<const types::RecordInfo*>( t )->parent_id );
        }
        return false;
    }

    template<typename T>
    bool is_a() const noexcept
    {
        return is_a( types::detail::type_id<T>() );
    }

    bool is_ref() const noexcept
    {
        return type && type->is_ref();
    }

    // -----------------------------------------------------------------

    template<typename T>
    auto try_as() const noexcept -> std::conditional_t<IsConst, const T*, T*>
    {
        using ReturnType = std::conditional_t<IsConst, const T*, T*>;
        return is_a<T>() ? static_cast<ReturnType>( ptr ) : nullptr;
    }

    template<typename T>
    auto& as() const
    {
        auto p = try_as<T>();
        NC_ASSERT_MSG( p, "TypedPtr type mismatch" );
        return *p;
    }

    /** Follow one handle. Returns the pointee with its DYNAMIC type, or null. */
    BasicTypedPtr deref() const noexcept
    {
        if (!is_ref())
            return {};
        const types::IRefOps* ops = static_cast<const types::RecordInfo*>( type )->as_ref();
        // IRefOps predates const-correct views and only exposes void* slots. The
        // operations used here (get / dynamic_type) are read-only, so following a
        // const handle is safe: the result stays const through `Pointer`.
        void* slot = const_cast<void*>( ptr );
        void* t    = ops->get( slot );
        return t ? BasicTypedPtr{ ops->dynamic_type( slot ), t } : BasicTypedPtr{};
    }

    // -----------------------------------------------------------------

    /**
     * @brief View of the element at @p p_index of a homogenously typed block.
     *
     * The pointer this view was made from points at the first element; elements
     * are assumed to be tightly packed at `type->size` strides.
     */
    BasicTypedPtr at( size_t p_index ) const noexcept
    {
        if (!type || !ptr)
            return {};
        if constexpr (IsConst) {
            auto* base = static_cast<const std::byte*>( ptr );
            return { type, static_cast<const void*>( base + p_index * type->size ) };
        } else {
            auto* base = static_cast<std::byte*>( ptr );
            return { type, static_cast<void*>( base + p_index * type->size ) };
        }
    }

    // -----------------------------------------------------------------

    BasicTypedPtr field( const types::FieldInfo& f ) const noexcept
    {
        Pointer slot = f.get_void_ptr( ptr );
        if (f.qualifier.is_pointer()) {
            if constexpr (IsConst) {
                slot = *static_cast<const void* const*>( slot );
            } else {
                slot = *static_cast<void**>( slot );
            }
            if (!slot)
                return {};
            const types::TypeInfo* t = f.get_type();
            if (f.qualifier.is_ref && t && t->is_record())
                t = static_cast<const types::RecordInfo*>( t )->dynamic_record( slot );
            return { t, slot };
        }
        return { f.get_type(), slot };
    }

    BasicTypedPtr field( StringView name ) const noexcept
    {
        if (!*this || !type->is_record())
            return {};
        for (auto r = static_cast<const types::RecordInfo*>( type ); r;) {
            if (auto f = r->find_field( name ))
                return field( *f );
            auto p = TypeDB::find( r->parent_id );
            r      = p && p->is_record() ? static_cast<const types::RecordInfo*>( p ) : nullptr;
        }
        return {};
    }

    BasicTypedPtr operator[]( StringView name ) const noexcept
    {
        return field( name );
    }

    // -------------------------------------------------------------------------

    bool assign( BasicTypedPtr p_src ) const
    {
        if (!*this || !p_src)
            return false;
        if (type->id == p_src.type->id) {
            type->replace( p_src.ptr, ptr );
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------------

    bool operator==( const BasicTypedPtr& o ) const noexcept
    {
        return ptr == o.ptr && type == o.type;
    }

    const types::TypeInfo* get_type() const noexcept
    {
        return type;
    }

    Pointer get_ptr() const noexcept
    {
        return ptr;
    }

private:
    const types::TypeInfo* type = nullptr;
    Pointer ptr                 = nullptr;
};

/**
 * @brief TypedPtr is a type-erased pointer to a value of a reflected type.
 */
using TypedPtr = BasicTypedPtr<false>;
/**
 * @brief ConstTypedPtr is a type-erased pointer to a const value of a reflected type.
 */
using ConstTypedPtr = BasicTypedPtr<true>;

// ---------------------------------------------------------------------------
// Numeric conversion
// ---------------------------------------------------------------------------

namespace detail {

template<typename T>
inline T clamp_cast( double v ) noexcept
{
    if constexpr (std::is_same_v<T, bool>) {
        return v != 0.0;
    } else if constexpr (std::is_integral_v<T>) {
        constexpr double lo = static_cast<double>( std::numeric_limits<T>::lowest() );
        constexpr double hi = static_cast<double>( std::numeric_limits<T>::max() );
        if (v <= lo)
            return std::numeric_limits<T>::lowest();
        if (v >= hi)
            return std::numeric_limits<T>::max();
        return static_cast<T>( v );
    } else {
        return static_cast<T>( v );
    }
}

/**
 * @brief Read a primitive value as a double.
 * @return false when the type is not a primitive (records, strings, enums...).
 */
inline bool read_numeric( const types::TypeInfo* p_type, const void* p_src, double& p_out ) noexcept
{
    switch (p_type->kind) {
        case types::TypeKind::BOOL:
            p_out = *static_cast<const bool*>( p_src ) ? 1.0 : 0.0;
            return true;
        case types::TypeKind::INT8:
            p_out = static_cast<double>( *static_cast<const int8_t*>( p_src ) );
            return true;
        case types::TypeKind::UINT8:
            p_out = static_cast<double>( *static_cast<const uint8_t*>( p_src ) );
            return true;
        case types::TypeKind::INT16:
            p_out = static_cast<double>( *static_cast<const int16_t*>( p_src ) );
            return true;
        case types::TypeKind::UINT16:
            p_out = static_cast<double>( *static_cast<const uint16_t*>( p_src ) );
            return true;
        case types::TypeKind::INT32:
            p_out = static_cast<double>( *static_cast<const int32_t*>( p_src ) );
            return true;
        case types::TypeKind::UINT32:
            p_out = static_cast<double>( *static_cast<const uint32_t*>( p_src ) );
            return true;
        case types::TypeKind::INT64:
            p_out = static_cast<double>( *static_cast<const int64_t*>( p_src ) );
            return true;
        case types::TypeKind::UINT64:
            p_out = static_cast<double>( *static_cast<const uint64_t*>( p_src ) );
            return true;
        case types::TypeKind::FLOAT:
            p_out = static_cast<double>( *static_cast<const float*>( p_src ) );
            return true;
        case types::TypeKind::DOUBLE:
            p_out = *static_cast<const double*>( p_src );
            return true;
        default:
            return false;
    }
}

/**
 * @brief Write a double into a primitive destination, clamping integer overflow.
 * @return false when the destination type is not a primitive.
 */
inline bool write_numeric( const types::TypeInfo* p_type, void* p_dst, double p_value ) noexcept
{
    switch (p_type->kind) {
        case types::TypeKind::BOOL:
            *static_cast<bool*>( p_dst ) = p_value != 0.0;
            return true;
        case types::TypeKind::INT8:
            *static_cast<int8_t*>( p_dst ) = clamp_cast<int8_t>( p_value );
            return true;
        case types::TypeKind::UINT8:
            *static_cast<uint8_t*>( p_dst ) = clamp_cast<uint8_t>( p_value );
            return true;
        case types::TypeKind::INT16:
            *static_cast<int16_t*>( p_dst ) = clamp_cast<int16_t>( p_value );
            return true;
        case types::TypeKind::UINT16:
            *static_cast<uint16_t*>( p_dst ) = clamp_cast<uint16_t>( p_value );
            return true;
        case types::TypeKind::INT32:
            *static_cast<int32_t*>( p_dst ) = clamp_cast<int32_t>( p_value );
            return true;
        case types::TypeKind::UINT32:
            *static_cast<uint32_t*>( p_dst ) = clamp_cast<uint32_t>( p_value );
            return true;
        case types::TypeKind::INT64:
            *static_cast<int64_t*>( p_dst ) = clamp_cast<int64_t>( p_value );
            return true;
        case types::TypeKind::UINT64:
            *static_cast<uint64_t*>( p_dst ) = clamp_cast<uint64_t>( p_value );
            return true;
        case types::TypeKind::FLOAT:
            *static_cast<float*>( p_dst ) = static_cast<float>( p_value );
            return true;
        case types::TypeKind::DOUBLE:
            *static_cast<double*>( p_dst ) = p_value;
            return true;
        default:
            return false;
    }
}

} // namespace detail

/**
 * @brief Copy @p p_src into @p p_dst, converting between primitive kinds.
 *
 * Same type: element-wise copy, requires the counts to match. Different types:
 * both sides must be primitives (`TypeInfo::is_primitive()`), in which case the
 * values are converted through double. Records, strings and enums only copy to
 * their own type. Integer destinations clamp instead of wrapping.
 *
 * @note Conversion goes through double, so integers beyond 2^53 lose precision.
 *       Material uniforms are 32-bit today, which this is more than enough for.
 * @return false when nothing was written (type mismatch, null views, zero count).
 */
inline bool coerce_value( TypedPtr p_dst, size_t p_dst_count, ConstTypedPtr p_src, size_t p_src_count )
{
    if (!p_dst || !p_src)
        return false;

    const types::TypeInfo* dst_type = p_dst.get_type();
    const types::TypeInfo* src_type = p_src.get_type();
    if (!dst_type || !src_type)
        return false;

    const size_t n = std::min( p_dst_count, p_src_count );
    if (n == 0)
        return false;

    if (dst_type->id == src_type->id) {
        for (size_t i = 0; i < n; ++i)
            dst_type->replace( p_src.at( i ).get_ptr(), p_dst.at( i ).get_ptr() );
        return true;
    }

    if (!dst_type->is_primitive() || !src_type->is_primitive())
        return false;

    for (size_t i = 0; i < n; ++i) {
        double value = 0.0;
        if (!detail::read_numeric( src_type, p_src.at( i ).get_ptr(), value ))
            return false;
        if (!detail::write_numeric( dst_type, p_dst.at( i ).get_ptr(), value ))
            return false;
    }
    return true;
}

/**
 * @brief Scalar convenience overload of coerce_value().
 */
inline bool coerce_value( TypedPtr p_dst, ConstTypedPtr p_src )
{
    return coerce_value( p_dst, 1, p_src, 1 );
}

// ---------------------------------------------------------------------------
// TypedVar
// ---------------------------------------------------------------------------

/**
 * @brief TypedVar is a data type to store a value of any type that has been
 * registered with the TypeDB. This is conceptually similar to std::any,
 * Godot's Variant, or C#'s object.
 *
 * TypedVar implements boxing/unboxing and small-buffer optimization for small
 * types, at least 32 bytes in size.
 *
 * A TypedVar holds either one value or a tightly-packed run of `count()`
 * homogeneously typed values (a C array). The latter exists so fixed-size
 * arrays of reflected types (e.g. `Vec4f[8]` for a `float4 mapScales[8]`
 * uniform) can be boxed without registering a container type per element type.
 *
 * This container is directly integrated with NCORE's reflection system, which
 * means that an instance of TypeInfo is embedded within it for runtime type
 * information.
 */
class TypedVar {
    static constexpr size_t SBO_SIZE  = 32;
    static constexpr size_t SBO_ALIGN = 16;

public:
    TypedVar() = default;

    template<typename T>
        requires(
            !std::same_as<std::remove_cvref_t<T>, TypedVar> && !std::same_as<std::remove_cvref_t<T>, TypedPtr> &&
            !std::is_array_v<std::remove_cvref_t<T>>
        )
    TypedVar( T&& v )
    {
        create<std::remove_cvref_t<T>>( std::forward<T>( v ) );
    }

    /**
     * @brief Deep copy of whatever the view points at.
     */
    TypedVar( TypedPtr src )
    {
        copy_from( src, 1 );
    }

    static TypedVar default_of( const types::TypeInfo* t )
    {
        TypedVar a;
        if (t) {
            a.num_elements = 1;
            a.ptr          = a.alloc( t, 1 );
            t->construct( a.ptr, nullptr );
            a.type = t;
        }
        return a;
    }

    /**
     * @brief A run of @p p_count default-constructed values of @p t.
     */
    static TypedVar array_of( const types::TypeInfo* t, size_t p_count )
    {
        TypedVar a;
        if (!t || p_count == 0)
            return a;
        a.num_elements = p_count;
        a.ptr          = a.alloc( t, p_count );
        for (size_t i = 0; i < p_count; ++i)
            t->construct( static_cast<std::byte*>( a.ptr ) + i * t->size, nullptr );
        a.type = t;
        return a;
    }

    /**
     * @brief Copy @p p_count values of @p t from @p p_data (tightly packed).
     */
    static TypedVar from_array( const types::TypeInfo* t, const void* p_data, size_t p_count )
    {
        TypedVar a;
        if (!t || !p_data || p_count == 0)
            return a;

        a.num_elements = p_count;
        a.ptr          = a.alloc( t, p_count );

        auto* src = static_cast<const std::byte*>( p_data );
        for (size_t i = 0; i < p_count; ++i)
            t->construct( static_cast<std::byte*>( a.ptr ) + i * t->size, src + i * t->size );

        a.type = t;
        return a;
    }

    /**
     * @brief Same as from_array(), named for callers passing an untyped buffer.
     */
    static TypedVar from_bytes( const types::TypeInfo* t, const void* p_data, size_t p_count )
    {
        return from_array( t, p_data, p_count );
    }

    /**
     * @brief Copy a C array of a reflected element type.
     */
    template<typename T, size_t N>
    static TypedVar from_array( const T ( &p_values )[N] )
    {
        const types::TypeInfo* ti = TypeDB::find<T>();
        NC_ASSERT_MSG( ti, "Array element type not reflected" );
        return from_array( ti, p_values, N );
    }

    TypedVar( const TypedVar& o )
    {
        copy_from( o.view(), o.num_elements );
    }
    TypedVar( TypedVar&& o ) noexcept
    {
        move_from( o );
    }

    TypedVar& operator=( const TypedVar& o )
    {
        if (this == &o)
            return *this;
        if (type && type->id == o.type->id && num_elements == o.num_elements) {
            for (size_t i = 0; i < num_elements; ++i)
                type->replace( o.element_ptr( i ), element_ptr( i ) );
        } else {
            reset();
            copy_from( o.view(), o.num_elements );
        }
        return *this;
    }
    TypedVar& operator=( TypedVar&& o ) noexcept
    {
        if (this != &o) {
            reset();
            move_from( o );
        }
        return *this;
    }
    template<typename T>
        requires(
            !std::same_as<std::remove_cvref_t<T>, TypedVar> && !std::same_as<std::remove_cvref_t<T>, TypedPtr> &&
            !std::is_array_v<std::remove_cvref_t<T>>
        )
    TypedVar& operator=( T&& v )
    {
        create<std::remove_cvref_t<T>>( std::forward<T>( v ) );
        return *this;
    }

    ~TypedVar()
    {
        reset();
    }

    bool has_value() const noexcept
    {
        return type != nullptr;
    }
    explicit operator bool() const noexcept
    {
        return has_value();
    }
    const types::TypeInfo* get_type() const noexcept
    {
        return type;
    }

    /**
     * @brief True when the held value is a reference-counted handle (Ref<T>).
     */
    bool is_ref() const noexcept
    {
        return type && type->is_ref();
    }

    /**
     * @brief Follow one handle, if the held value is a Ref<T>.
     *
     * @return A view of the pointee with its DYNAMIC type, or an empty view when
     *         the value is not a ref or the handle is null.
     */
    ConstTypedPtr deref() const noexcept
    {
        return view().deref();
    }

    /**
     * @brief Number of homogeneously typed values held (1 for a scalar).
     */
    size_t count() const noexcept
    {
        return num_elements;
    }

    bool is_array() const noexcept
    {
        return num_elements > 1;
    }

    /**
     * @brief Bytes occupied by the packed values (not the GPU-side layout).
     */
    size_t size_bytes() const noexcept
    {
        return type ? type->size * num_elements : 0;
    }

    /**
     * @brief Pointer to element @p p_index, or null when out of range.
     */
    void* element_ptr( size_t p_index ) noexcept
    {
        if (!type || !ptr || p_index >= num_elements)
            return nullptr;
        return static_cast<std::byte*>( ptr ) + p_index * type->size;
    }
    const void* element_ptr( size_t p_index ) const noexcept
    {
        if (!type || !ptr || p_index >= num_elements)
            return nullptr;
        return static_cast<const std::byte*>( ptr ) + p_index * type->size;
    }

    void reset() noexcept
    {
        if (!type)
            return;
        for (size_t i = 0; i < num_elements; ++i)
            type->destruct( element_ptr( i ) );
        if (!is_inline() && ptr)
            memfree_align( ptr, type->alignment );
        type         = nullptr;
        ptr          = nullptr;
        num_elements = 1;
    }

    // -----------------------------------------------------------------

    template<typename T, typename... Args>
    T& create( Args&&... args )
    {
        reset();
        const types::TypeInfo* ti = TypeDB::find<T>();
        NC_ASSERT_MSG( ti, "Type not reflected" );
        NC_ASSERT_MSG( ti->size == sizeof( T ), "Registered size mismatch" );

        void* mem = alloc( ti, 1 );
        T* t      = static_cast<T*>( mem );
        try {
            new ( t ) T( std::forward<Args>( args )... );
        } catch (...) {
            if (mem != buf)
                memfree_align( mem, ti->alignment );
            throw;
        }
        type         = ti;
        ptr          = mem;
        num_elements = 1;
        return *t;
    }

    /**
     * @brief Check if the held value is of type T.
     * @return false if the TypedVar is empty.
     */
    template<typename T>
    bool is() const noexcept
    {
        return type && type->id == types::detail::type_id<T>();
    }
    template<typename T>
    T* try_get() noexcept
    {
        return is<T>() ? static_cast<T*>( ptr ) : nullptr;
    }
    template<typename T>
    const T* try_get() const noexcept
    {
        return is<T>() ? static_cast<const T*>( ptr ) : nullptr;
    }
    template<typename T>
    T& get()
    {
        auto p = try_get<T>();
        NC_ASSERT_MSG( p, "Type mismatch" );
        return *p;
    }
    template<typename T>
    const T& get() const
    {
        auto p = try_get<T>();
        NC_ASSERT_MSG( p, "Type mismatch" );
        return *p;
    }
    template<typename T>
    T value_or( T fallback ) const
    {
        auto p = try_get<T>();
        return p ? *p : std::move( fallback );
    }

    /**
     * Replace the held value (and type) with a deep copy of what `p_src` points at.
     * @param p_src source value to copy. If null, clears the TypedVar.
     */
    void set( TypedPtr p_src )
    {
        if (!p_src) {
            reset();
            return;
        }

        // p_src may point inside our own storage (e.g. a.set( a["child"] )).
        // reset() would destroy the source before we copy it, so copy out first.
        if (aliases_self( p_src )) {
            TypedVar tmp( p_src );
            *this = std::move( tmp );
            return;
        }

        if (type && p_src.get_type() && type->id == p_src.get_type()->id && num_elements == 1) {
            if (ptr != p_src.get_ptr())
                type->replace( p_src.get_ptr(), ptr ); // reuse storage, no realloc
            return;
        }

        reset();
        copy_from( p_src, 1 );
    }

    TypedVar& operator=( TypedPtr src )
    {
        set( src );
        return *this;
    }

    /**
     * Keep the current type and convert instead.
     * @return false if the types are incompatible.
     */
    bool assign( TypedPtr src )
    {
        return has_value() && num_elements == 1 && view().assign( src );
    }

    /**
     * Keep the current type and coercion-convert instead.
     * @return false if the types are incompatible.
     */
    bool coerce( ConstTypedPtr src, size_t p_count )
    {
        return has_value() && coerce_value( view(), num_elements, src, p_count );
    }

    // -----------------------------------------------------------------

    TypedPtr view() noexcept
    {
        return { type, ptr };
    }
    ConstTypedPtr view() const noexcept
    {
        return { type, ptr };
    }

    ConstTypedPtr operator[]( StringView p_name ) const noexcept
    {
        return view().field( p_name );
    }

private:
    bool fits_inline( const types::TypeInfo* p_type, size_t p_count ) const noexcept
    {
        return p_type->size * p_count <= SBO_SIZE && p_type->alignment <= SBO_ALIGN;
    }
    bool is_inline() const noexcept
    {
        return ptr == buf;
    }

    void* alloc( const types::TypeInfo* p_type, size_t p_count )
    {
        if (fits_inline( p_type, p_count ))
            return buf;
        return memalloc_aligned( p_type->size * p_count, p_type->alignment ? p_type->alignment : 1 );
    }

    void copy_from( ConstTypedPtr p_src, size_t p_count )
    {
        if (!p_src || p_count == 0)
            return;
        type         = p_src.get_type();
        num_elements = p_count;
        ptr          = alloc( type, num_elements );
        for (size_t i = 0; i < num_elements; ++i)
            type->construct( static_cast<std::byte*>( ptr ) + i * type->size, p_src.at( i ).get_ptr() );
    }

    void move_from( TypedVar& p_var ) noexcept
    {
        if (!p_var.type)
            return;
        type         = p_var.type;
        num_elements = p_var.num_elements;
        if (!p_var.is_inline()) { // steal the pointer
            ptr                = p_var.ptr;
            p_var.type         = nullptr;
            p_var.ptr          = nullptr;
            p_var.num_elements = 1;
        } else { // no move op in IValueOps, so clone + destroy
            ptr = buf;
            for (size_t i = 0; i < num_elements; ++i)
                type->clone(
                    static_cast<const std::byte*>( p_var.ptr ) + i * type->size,
                    static_cast<std::byte*>( ptr ) + i * type->size
                );
            p_var.reset();
        }
    }

    bool aliases_self( TypedPtr src ) const noexcept
    {
        if (!type || !ptr)
            return false;
        auto p = static_cast<const std::byte*>( src.get_ptr() );
        auto b = static_cast<const std::byte*>( ptr );
        return p >= b && p < b + size_bytes();
    }

private:
    alignas( SBO_ALIGN ) std::byte buf[SBO_SIZE];
    const types::TypeInfo* type = nullptr;
    void* ptr                   = nullptr;
    size_t num_elements         = 1;
};

} // namespace nc
