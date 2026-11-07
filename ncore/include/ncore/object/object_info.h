// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: object_info.h - information about object types.

#pragma once

#include "object.h"
#include "reference.h"

namespace nc::types {

/**
 * @brief A RecordInfo with whole-object lifecycle for T.
 *
 * Construct/destruct/clone/replace operate on the C++ object as a
 * whole instead of walking reflected fields.
 */
template<typename T>
struct ObjectClass : public TypedLifecycle<T, RecordInfo> {
    using TypedLifecycle<T, RecordInfo>::TypedLifecycle;

    /**
     * @brief Allocate a fresh T owned by one ref count, as its RefCounted base.
     */
    RefCounted* create_refcounted() const override
    {
        if constexpr (
            std::derived_from<T, ::nc::RefCounted> && std::default_initializable<T> && !std::is_abstract_v<T>
        ) {
            T* p = ::nc::Ref<T>::create().detach();
            // The Ref view of this object (RefClass::get) hands out T* and treats it as
            // the most-derived address, so the RefCounted base must sit at offset 0.
            NC_ASSERT_MSG(
                static_cast<void*>( static_cast<::nc::RefCounted*>( p ) ) == static_cast<void*>( p ),
                "Ref targets must not use multiple inheritance with RefCounted off offset 0"
            );
            return p;
        }
        return nullptr;
    }

    const RecordInfo* dynamic_record( const void* p ) const noexcept override
    {
        if constexpr (std::derived_from<T, ::nc::Object>)
            return &static_cast<const T*>( p )->get_class_info(); // virtual; most-derived NCLASS
        return this;
    }
};

// ------------------------------------------------------------------------------------

/**
 * @brief IRefOps defines the interface for operations on a Ref<T> slot, with T erased.
 */
struct IRefOps {
    virtual ~IRefOps() = default;

    virtual const RecordInfo* target_type() const noexcept             = 0; // static T
    virtual void* get( void* ref ) const noexcept                      = 0; // T* (null if empty)
    virtual RefCounted* get_refcounted( void* ref ) const noexcept     = 0;
    virtual const RecordInfo* dynamic_type( void* ref ) const noexcept = 0; // most-derived, or T
    virtual void reset( void* ref ) const noexcept                     = 0;

    /**
     * @brief Make `ref` share `target` (acquires).
     * @return false if target is not a T (checked with is_a).
     */
    virtual bool share( void* ref, RefCounted* target ) const noexcept = 0;

    /**
     * @brief Make `ref` own a fresh default-constructed `dyn` (or T when null).
     * @return false if impossible.
     */
    virtual bool create( void* ref, const RecordInfo* dyn = nullptr ) const = 0;
};

template<typename RefT>
struct RefClass final : TypedLifecycle<RefT, RecordInfo>, IRefOps {
    using T = typename RefT::element_type;
    static_assert( std::derived_from<T, ::nc::RefCounted> );

    RefClass( const char* n, TypeId i, size_t sz, size_t al ) : TypedLifecycle<RefT, RecordInfo>( n, i, sz, al )
    {
        this->kind = TypeKind::REF;
    }

    const IRefOps* as_ref() const noexcept override
    {
        return this;
    }

    // --------------------------------------------------------------------------------

    void to_string( String& out, const void* r ) const override
    {
        out += static_cast<const RefT*>( r )->get() ? "Ref(set)" : "Ref(null)";
    }

    // --------------------------------------------------------------------------------

    const RecordInfo* target_type() const noexcept override
    {
        if (!target) {
            const TypeInfo* t = TypeDB::find( detail::type_id<T>() );
            target            = t && t->is_record() ? static_cast<const RecordInfo*>( t ) : nullptr;
        }
        return target;
    }

    void* get( void* r ) const noexcept override
    {
        return static_cast<RefT*>( r )->get();
    }

    RefCounted* get_refcounted( void* r ) const noexcept override
    {
        return static_cast<RefCounted*>( static_cast<RefT*>( r )->get() );
    }

    const RecordInfo* dynamic_type( void* r ) const noexcept override
    {
        const RecordInfo* t = target_type();
        T* p                = static_cast<RefT*>( r )->get();
        return p && t ? t->dynamic_record( p ) : t;
    }

    void reset( void* r ) const noexcept override
    {
        static_cast<RefT*>( r )->reset();
    }

    bool share( void* r, RefCounted* tgt ) const noexcept override
    {
        if (!tgt) {
            reset( r );
            return true;
        }
        if (!tgt->is_a( detail::type_id<T>() ))
            return false;
        *static_cast<RefT*>( r ) = RefT::share( static_cast<T*>( tgt ) );
        return true;
    }

    bool create( void* r, const RecordInfo* dyn = nullptr ) const override
    {
        const RecordInfo* want = dyn ? dyn : target_type();
        if (!want || !TypeDB::is_derived_or_same( want, target_type() ))
            return false;
        RefCounted* rc = want->create_refcounted();
        if (!rc)
            return false;
        *static_cast<RefT*>( r ) = RefT( static_cast<T*>( rc ) );
        return true;
    }

private:
    mutable const RecordInfo* target = nullptr;
};

} // namespace nc::types

//------------------------------------------------------------------------------

#define NCLASS( class_name, parent_class )                                                                              \
public:                                                                                                                 \
    const ::nc::StringView get_class_name() const override                                                              \
    {                                                                                                                   \
        return #class_name;                                                                                             \
    }                                                                                                                   \
    ::nc::types::TypeId get_type_id() const override                                                                    \
    {                                                                                                                   \
        return ::nc::TypeDB::get_type_id<class_name>();                                                                 \
    }                                                                                                                   \
    const ::nc::types::RecordInfo& get_class_info() const override                                                      \
    {                                                                                                                   \
        return RECORD_INFO_##class_name();                                                                              \
    }                                                                                                                   \
                                                                                                                        \
private:                                                                                                                \
    using Self = class_name;                                                                                            \
    inline static auto RECORD_INFO_##class_name() -> ::nc::types::RecordInfo&                                           \
    {                                                                                                                   \
        ::nc::types::RecordInfo& ci_##class_name = []() -> ::nc::types::RecordInfo& {                                   \
            auto& c     = ::nc::TypeDB::register_type<::nc::types::ObjectClass<class_name>, class_name>( #class_name ); \
            c.parent_id = ::nc::TypeDB::get_type_id<parent_class>();                                                    \
            return c;                                                                                                   \
        }();                                                                                                            \
        return ci_##class_name;                                                                                         \
    }                                                                                                                   \
    inline static const int REGISTER_##class_name = ( RECORD_INFO_##class_name(), 0 );

//------------------------------------------------------------------------------

#define NC_PROPS_BEGIN()                                                                                               \
    static const ::nc::types::FieldInfo* nc_get_fields_( size_t& out_count )                                           \
    {                                                                                                                  \
        static const ::nc::types::FieldInfo fields[] = {

#define ADD_PROPERTY_IMPL( Member, Flags, ... )                                                                        \
    []() -> ::nc::types::FieldInfo {                                                                                   \
        using MemberType = decltype( Self::Member );                                                                   \
        return ::nc::types::FieldInfo{ #Member,                                                                        \
                                       ::nc::types::detail::field_type_id<MemberType>(),                               \
                                       sizeof( MemberType ),                                                           \
                                       offsetof( Self, Member ),                                                       \
                                       Flags,                                                                          \
                                       ::nc::types::detail::field_qualifier<MemberType>() };                           \
    }(),

#define ADD_PROPERTY( ... )                                                                                            \
    ADD_PROPERTY_IMPL( __VA_ARGS__, ::nc::types::PropertyFlags::SERIALIZABLE | ::nc::types::PropertyFlags::EDITABLE )

#define NC_PROPS_END()                                                                                                 \
    }                                                                                                                  \
    ;                                                                                                                  \
    out_count = sizeof( fields ) / sizeof( fields[0] );                                                                \
    return fields;                                                                                                     \
    }                                                                                                                  \
                                                                                                                       \
    inline static void FIELDS_INFO_INIT()                                                                              \
    {                                                                                                                  \
        size_t count = 0;                                                                                              \
        auto* begin  = nc_get_fields_( count );                                                                        \
        ::nc::TypeDB::provide_fields( RECORD_INFO().id, begin, begin + count );                                        \
    }                                                                                                                  \
    inline static const int REGISTER_FIELDS_INFO = ( FIELDS_INFO_INIT(), 0 );

//------------------------------------------------------------------------------

/**
 * @brief Registers a record type in TypeDB (ObjectClass lifecycle).
 * Place inside the struct/union body.
 *
 * Prefer NC_COMPONENT / NC_COMPONENT_API for Flecs-facing types.
 * NSTRUCT_V remains for non-ECS value types (Quaternion, Color, …).
 * Use NPROPS_* macros to register fields, or REFLECT + reflection_gen.
 */
#define NSTRUCT_V( T )                                                                                                 \
    using Self = T;                                                                                                    \
    inline static ::nc::types::RecordInfo& RECORD_INFO()                                                               \
    {                                                                                                                  \
        static ::nc::types::RecordInfo& ci = []() -> ::nc::types::RecordInfo& {                                        \
            auto& c = ::nc::TypeDB::register_type<::nc::types::ObjectClass<T>, T>( #T );                               \
            return c;                                                                                                  \
        }();                                                                                                           \
        return ci;                                                                                                     \
    }                                                                                                                  \
    inline static const int REGISTER_##T = ( RECORD_INFO(), 0 );                                                       \
    const ::nc::types::RecordInfo& get_class_info() const                                                              \
    {                                                                                                                  \
        return RECORD_INFO();                                                                                          \
    }
