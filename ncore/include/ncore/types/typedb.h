// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

#include "typeinfo.h"

namespace nc {

/**
 * @brief A global registry of reflected types and classes.
 * Equivalent to Godot's ClassDB.
 */
class NCAPI TypeDB {
public:
    TypeDB( const TypeDB& )            = delete;
    TypeDB& operator=( const TypeDB& ) = delete;

    TypeDB( TypeDB&& )            = delete;
    TypeDB& operator=( TypeDB&& ) = delete;

    static TypeDB& get_instance()
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
        static TypeDB instance;
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
    template<std::derived_from<types::TypeInfo> TInfo, typename TRefl, typename... TArgs>
    static TInfo& register_type( const char* name, TArgs&&... extra ) noexcept
    {
        static TInfo& info = [&]() -> TInfo& {
            static TInfo instance(
                name, types::detail::type_id<TRefl>(), sizeof( TRefl ), alignof( TRefl ),
                std::forward<TArgs>( extra )...
            );
            instance.next  = type_list_head;
            type_list_head = &instance;
            // Seed type_cache so find<T>() resolves without a list walk.
            // Always overwrite: NSTRUCT_V/NCLASS/ECS_COMPONENT ObjectClass must
            // win over a plain gen RecordInfo registered under the same TypeId.
            auto& cache = get_instance().type_cache;
            auto it     = cache.find( instance.id );
            if (it != cache.end()) {
                // Steal fields from a gen-emitted plain RecordInfo (same TypeId)
                // so the winner keeps Inspector metadata when static-init order
                // registers gen first.
                auto* old_rec = dynamic_cast<types::RecordInfo*>( it->second );
                auto* new_rec = dynamic_cast<types::RecordInfo*>( &instance );
                if (old_rec && new_rec && !new_rec->fields_begin && old_rec->fields_begin) {
                    new_rec->fields_begin = old_rec->fields_begin;
                    new_rec->fields_end   = old_rec->fields_end;
                }
            }
            apply_pending_fields( instance );
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
    static types::TypeInfo& register_type( const char* name ) noexcept
    {
        static types::TypeInfo& info = [&]() -> types::TypeInfo& {
            static types::TTypeInfo<T> instance(
                name, types::detail::type_id_from_name( name ), sizeof( T ), alignof( T )
            );
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
    static void register_type( types::TypeInfo* p_info ) noexcept;

    /**
     * @brief Attach (or defer) field metadata for a TypeId.
     *
     * Used by reflection_gen for NC_COMPONENT types so fields land on the
     * winning ObjectClass regardless of static-init order. If the type is not
     * registered yet, fields are stashed and applied in initialize() (and when
     * register_type<TInfo,TRefl> later creates the ObjectClass).
     *
     * Never overwrites non-empty fields (NPROPS / earlier provide_fields win).
     */
    static void provide_fields( types::TypeId id, const types::FieldInfo* begin, const types::FieldInfo* end ) noexcept;

    // TODO: add register_class<T>() helper method

    //------------------------------------------------------------------------------

    static const types::TypeInfo* find( types::TypeId id ) noexcept
    {
        if (!id.valid())
            return nullptr;

        auto& map = get_instance().type_cache;
        auto it   = map.find( id );
        if (it != map.end()) {
            type_query_hits++;
            return it->second;
        }

        for (auto* c = type_list_head; c; c = c->next) {
            type_query_hits++;
            if (c->id == id) {
                map[id] = c;
                NC_LOG_DEBUG( "TypeDB: cached type name='{}' with ID={}", c->name, id.value );
                return c;
            }
        }

        NC_LOG_WARN( "TypeDB: type ID={} not found, has it been reflected?", id.value );
        return nullptr;
    }

    static const types::TypeInfo* find( StringView name ) noexcept
    {
        for (auto* c = type_list_head; c; c = c->next) {
            type_query_hits++;
            if (name == c->name)
                return c;
        }

        NC_LOG_WARN( "TypeDB: type name={} not found, has it been reflected?", name );
        return nullptr;
    }

    static const types::RecordInfo* find_record( types::TypeId id ) noexcept
    {
        const types::TypeInfo* t = find( id );
        if (!t)
            return nullptr;
        if (!t->is_record()) {
            NC_LOG_WARN( "Regsitry: type name={} is found but is not a record type", t->name );
            return nullptr;
        }
        return static_cast<const types::RecordInfo*>( t );
    }

    static const types::TypeInfo& get( types::TypeId id ) noexcept;
    static const types::TypeInfo& get( StringView name ) noexcept;

    static const StringView get_type_name( types::TypeId id ) noexcept
    {
        auto* c = find( id );
        return c ? c->name : "<unknown>";
    }

    /**
     * @brief Check if a type has been registered to TypeDB.
     */
    static bool contains( StringView name ) noexcept
    {
        for (auto* c = type_list_head; c; c = c->next) {
            type_query_hits++;
            if (std::strcmp( name.data(), c->name ) == 0)
                return true;
        }
        return false;
    }

    static void to_string( String& out, const void* instance, types::TypeId id ) noexcept
    {
        auto t = find( id );
        if (!t)
            out = "UnknownType";
        return t->to_string( out, instance );
    }

    /**
     * @brief True when @p p_want is @p p_base or derived from it.
     *
     * Walks the RecordInfo parent chain by id (resolved_parent is only wired up in
     * TypeDB::initialize(), so it cannot be trusted here).
     */
    static bool is_derived_or_same( const types::RecordInfo* p_want, const types::RecordInfo* p_base )
    {
        if (!p_want || !p_base)
            return false;

        for (const types::RecordInfo* r = p_want; r;) {
            if (r == p_base || r->id == p_base->id)
                return true;
            const types::TypeInfo* parent = find( r->parent_id );
            r = parent && parent->is_record() ? static_cast<const types::RecordInfo*>( parent ) : nullptr;
        }
        return false;
    }

    //------------------------------------------------------------------------------

    template<typename T>
    static const types::TypeInfo* find() noexcept
    {
        return find( types::detail::type_id<T>() );
    }

    template<typename T>
    static const types::RecordInfo* find_record() noexcept
    {
        return find_record( types::detail::type_id<T>() );
    }

    template<typename T>
    static bool contains() noexcept
    {
        types::TypeId id = types::detail::type_id<T>();
        for (auto* c = type_list_head; c; c = c->next) {
            type_query_hits++;
            if (id == c->id)
                return true;
        }
        return false;
    }

    /**
     * @brief Hard exits if we can't find the type info.
     */
    template<typename T>
    static const types::TypeInfo& get() noexcept
    {
        NC_ASSERT_MSG(
            contains<T>(),
            std::format( "Type '{}' is not found in the types registry.", types::detail::type_name<T>().data() ).c_str()
        );
        return get( types::detail::type_id<T>() );
    }

    /**
     * @return The hashed id of the type.
     */
    template<typename T>
    static const types::TypeId get_type_id() noexcept
    {
        return types::detail::type_id<T>();
    }

    template<typename T>
    static const StringView get_type_name()
    {
        return get_type_name( types::detail::type_id<T>() );
    }

    template<typename T>
    static const char* get_enum_name( const T* value ) noexcept
    {
        using RawType = std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>;
        return static_cast<const types::EnumInfo&>( get<RawType>() ).get_name( value ).data();
    }

    // hard-exits version, mirroring get<T>()
    template<typename T>
    static const types::RecordInfo& get_record() noexcept
    {
        const types::RecordInfo* c = find_record<T>();
        NC_ASSERT_MSG(
            c,
            std::format( "Type '{}' is not found in the types registry.", types::detail::type_name<T>().data() ).c_str()
        );
        return *c;
    }

    template<typename T>
    static void to_string( String& out, const void* instance )
    {
        return to_string( out, instance, types::detail::type_id<T>() );
    }

    static int get_types_hits()
    {
        return type_query_hits;
    }

private:
    TypeDB();

    static types::TypeInfo* type_list_head;
    static int type_query_hits;

    static void apply_pending_fields( types::TypeInfo& info ) noexcept;

    HashMap<types::TypeId, types::TypeInfo*> type_cache;
    // Deferred field ranges from provide_fields() before the TypeId is cached.
    HashMap<types::TypeId, std::pair<const types::FieldInfo*, const types::FieldInfo*>> pending_fields;
};

} // namespace nc

//------------------------------------------------------------------------------

/**
 * @brief Marks a field (or record) for reflection.
 *
 * REFLECT covers the common case (serializable + editable). REFLECT_HINT takes
 * extra PropertyFlags, additive to those defaults:
 *   REFLECT_HINT( types::PropertyFlags::HIDDEN )
 *   REFLECT_HINT( types::PropertyFlags::READ_ONLY )
 *   REFLECT_HINT( types::PropertyFlags::HIDDEN | types::PropertyFlags::READ_ONLY )
 *
 * The argument is stringified into the annotation and parsed by
 * reflection_gen; it is not evaluated, so an unknown token produces a
 * generator warning instead of a C++ error.
 */
#if __has_cpp_attribute( clang::annotate )
#define REFLECT             __attribute__( ( annotate( "Reflect" ) ) )
#define REFLECT_HINT( ... ) __attribute__( ( annotate( "Reflect " #__VA_ARGS__ ) ) )
#else
#define REFLECT
#define REFLECT_HINT( ... )
#endif

//------------------------------------------------------------------------------

#define NENUM_ELEMENT( EnumT, element )                                                                                \
    ::nc::types::EnumElement                                                                                           \
    {                                                                                                                  \
        #element, static_cast<int64_t>( EnumT::element )                                                               \
    }

#define NENUM( T, ... )                                                                                                  \
    inline static ::nc::types::EnumInfo& nc_enum_info_##T()                                                              \
    {                                                                                                                    \
        static ::nc::types::EnumElement nc_enum_elems_##T[] = { __VA_ARGS__ };                                           \
        static ::nc::types::EnumInfo& ei                    = []() -> ::nc::types::EnumInfo& {                           \
            auto& e          = ::nc::TypeDB::register_type<::nc::types::EnumInfo, T>( #T );                              \
            e.elements_begin = nc_enum_elems_##T;                                                                        \
            e.elements_end   = nc_enum_elems_##T + ( sizeof( nc_enum_elems_##T ) / sizeof( ::nc::types::EnumElement ) ); \
            e.is_unsigned    = std::is_unsigned_v<std::underlying_type_t<T>>;                                            \
            return e;                                                                                                    \
        }();                                                                                                             \
        return ei;                                                                                                       \
    }                                                                                                                    \
    inline static const int nc_trig_enum_##T = ( nc_enum_info_##T(), 0 );

//------------------------------------------------------------------------------

#define NENUM_GET_NAME( p ) ::nc::TypeDB::get_enum_name( p )
