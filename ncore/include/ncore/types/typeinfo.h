// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: typeinfo.h - type descriptors

#pragma once

#include <ncore.h>
#include <ncore/core/collection.h>

#include "types.h"

namespace nc {
class TypeDB;
class RefCounted;
} // namespace nc

namespace nc::types {

struct IRefOps;

struct NCAPI TypeInfo : public IValueOps {
    const char* name;
    TypeId id;
    size_t size;
    size_t alignment;
    TypeKind kind  = TypeKind::INVALID;
    TypeInfo* next = nullptr; // linked list, ptr to the next chain.

    TypeInfo() : name( nullptr ), id( TypeId::null() ), size( 0 ), alignment( 0 ) {}
    TypeInfo( const char* n, TypeId i, size_t sz, size_t align ) : name( n ), id( i ), size( sz ), alignment( align ) {}

    TypeInfo( const TypeInfo& )            = delete;
    TypeInfo& operator=( const TypeInfo& ) = delete;

    void construct( void* instance, const void* data = nullptr ) const override;
    void destruct( void* instance ) const override;
    void clone( const void* src, void* dst ) const override;
    void replace( const void* src, void* dst ) const override;

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

    bool is_ref() const noexcept
    {
        return kind == TypeKind::REF;
    }

    /**
     * @brief Erased operations for a Ref<T> handle slot, or null.
     */
    virtual const IRefOps* as_ref() const noexcept
    {
        return nullptr;
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
class NCAPI RecordInfo : public TypeInfo {
public:
    TypeId parent_id              = TypeId::null();
    const FieldInfo* fields_begin = nullptr;
    const FieldInfo* fields_end   = nullptr;

    RecordInfo() = default;
    RecordInfo( const char* type_name, TypeId t_id, size_t type_size, size_t align ) :
        TypeInfo( type_name, t_id, type_size, align )
    {
        kind = TypeKind::RECORD;
    }

    const RecordInfo* get_parent() const noexcept
    {
        return resolved_parent;
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

    // --------------------------------------------------------------------------

    virtual const RecordInfo* dynamic_record( const void* ) const noexcept;
    virtual RefCounted* create_refcounted() const;

    // --------------------------------------------------------------------------

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

private:
    friend class ::nc::TypeDB;
    const RecordInfo* resolved_parent = nullptr;
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

    /**
     * @brief Visit a reference-counted handle slot (TypeKind::REF).
     *
     * Called by RecordInfo::visit_field() instead of primitive() so archives
     * can persist a stable identity (path / RID) instead of raw pointer bytes.
     * Editors use it to display the pointee. The default is a no-op: a visitor
     * that does not understand refs leaves the slot untouched.
     */
    virtual void reference( const RecordInfo* c, void* instance )
    {
        ( void ) c;
        ( void ) instance;
    }

    /**
     * @brief Bulk-visit `count` contiguous elements of `elem_type` at `data`.
     *
     * Called by VectorClass::visit for primitive element types instead of the
     * per-element array_element()/primitive() loop. Resource archives override it
     * to read/write the whole run (e.g. a large byte buffer) in one operation. The
     * default preserves per-element semantics for other visitors.
     */
    virtual void primitive_array( const TypeInfo* elem_type, void* data, size_t count )
    {
        auto bytes = static_cast<uint8_t*>( data );
        for (size_t i = 0; i < count; ++i)
            primitive( elem_type, bytes + i * elem_type->size );
    }
};

/**
 * @brief Implements the IValueOps lifecycle (construct/destruct/clone/replace)
 * for a concrete C++ type T on top of any TypeInfo-derived Base.
 *
 * Operations T can't support fall back to Base's implementation, so a RecordInfo
 * base walks reflected fields and a plain TypeInfo base keeps its default.
 * The mixin knows nothing about records, refs or fields.
 */
template<typename T, typename Base>
struct TypedLifecycle : Base {
    using Base::Base; // inherit (name, id, size, align) ctor

    void construct( void* instance, const void* data = nullptr ) const override
    {
        if constexpr (!std::is_abstract_v<T>) {
            if (data) {
                if constexpr (std::is_copy_constructible_v<T>) {
                    ::new ( instance ) T( *static_cast<const T*>( data ) );
                    return;
                }
                // data given but T isn't copyable: don't silently default-construct.
                NC_ASSERT_MSG( false, "construct(data) on a non-copy-constructible type" );
            }
            if constexpr (std::is_default_constructible_v<T>) {
                ::new ( instance ) T();
                return;
            }
        }
        Base::construct( instance, data );
    }

    void destruct( void* instance ) const override
    {
        if constexpr (!std::is_abstract_v<T>)
            static_cast<T*>( instance )->~T();
        else
            Base::destruct( instance );
    }

    void clone( const void* src, void* dst ) const override
    {
        if constexpr (!std::is_abstract_v<T> && std::is_copy_constructible_v<T>)
            ::new ( dst ) T( *static_cast<const T*>( src ) );
        else
            Base::clone( src, dst );
    }

    void replace( const void* src, void* dst ) const override
    {
        if constexpr (!std::is_abstract_v<T> && std::is_copy_assignable_v<T>)
            *static_cast<T*>( dst ) = *static_cast<const T*>( src );
        else
            Base::replace( src, dst );
    }
};

} // namespace nc::types
