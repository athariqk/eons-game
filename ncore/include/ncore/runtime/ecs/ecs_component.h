#pragma once

#include <ncore/core/types.h>

namespace nc {

/**
 * @brief Represents a component as integer ID.
 */
using EcsComponent = uint64_t;

/**
 * @brief Base for Flecs-facing component / singleton / tag / event types.
 *
 * Registers T as RecordInfoT (whole-object lifecycle) under type_id<T>().
 * Field metadata is attached by reflection_gen via TypeRegistry::provide_fields
 * (REFLECT-annotated fields) and/or NPROPS.
 */
namespace detail {

// Null-terminated pretty name for NC_COMPONENT registration (no exit-time dtor).
template<std::size_t... Idx>
consteval std::array<char, sizeof...( Idx ) + 1> make_name_cstr( std::string_view name, std::index_sequence<Idx...> )
{
    return std::array<char, sizeof...( Idx ) + 1>{ name[Idx]..., '\0' };
}

template<typename T>
struct EcsComponentTypeName {
    static constexpr auto raw        = ::nc::rtti::detail::type_name<T>();
    static constexpr std::size_t len = raw.size() > 0 ? raw.size() - 1 : 0;
    static constexpr auto value      = make_name_cstr( raw.substr( 0, len ), std::make_index_sequence<len>{} );
};

template<typename T>
struct EcsComponentBase {
    using Self = T;

    inline static ::nc::rtti::RecordInfo& nc_info_()
    {
        static ::nc::rtti::RecordInfo& ci = ::nc::rtti::TypeRegistry::register_type<::nc::rtti::RecordInfoT<T>, T>(
            EcsComponentTypeName<T>::value.data()
        );
        return ci;
    }

    const ::nc::rtti::RecordInfo& get_class_info() const
    {
        return nc_info_();
    }
};

} // namespace detail

} // namespace nc

/**
 * @brief Declare an ECS component/singleton/tag/event (no DLL export).
 *
 * Named NC_COMPONENT (not ECS_COMPONENT) to avoid colliding with the
 * flecs C API macro `ECS_COMPONENT(world, id)`.
 *
 * Call site owns the braces — do not put `{` in the macro:
 * @code
 * NC_COMPONENT(FooBar)
 * {
 *     REFLECT int Value = 0;
 * };
 * @endcode
 */
#define NC_COMPONENT( N ) struct N : ::nc::detail::EcsComponentBase<N>

/** Same as NC_COMPONENT but exports the type from ncore (NCAPI). */
#define NC_COMPONENT_API( N ) struct NCAPI N : ::nc::detail::EcsComponentBase<N>
