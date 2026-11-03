#pragma once

#include <ncore/object/object_info.h>

namespace nc {

/**
 * @brief Represents a component as integer ID.
 */
using EcsComponent = uint64_t;

namespace detail {

// Null-terminated pretty name for NC_COMPONENT registration (no exit-time dtor).
template<std::size_t... Idx>
consteval std::array<char, sizeof...( Idx ) + 1> make_name_cstr( std::string_view name, std::index_sequence<Idx...> )
{
    return std::array<char, sizeof...( Idx ) + 1>{ name[Idx]..., '\0' };
}

template<typename T>
struct EcsComponentTypeName {
    static constexpr auto raw        = ::nc::types::detail::type_name<T>();
    static constexpr std::size_t len = raw.size() > 0 ? raw.size() - 1 : 0;
    static constexpr auto value      = make_name_cstr( raw.substr( 0, len ), std::make_index_sequence<len>{} );
};

} // namespace detail

/**
 * @brief Base for ECS components via CRTP.
 *
 * An ECS component is typically a POD struct with REFLECT-annotated fields.
 * Though, it can be any type that is default-constructible and copyable.
 *
 * This automatically registers the component type with the RTTI system and
 * provides a static method to retrieve its TypeInfo.
 */
template<typename T>
struct EcsComponentBase {
    using Self = T;

    inline static ::nc::types::RecordInfo& CLASS_INFO()
    {
        static ::nc::types::RecordInfo& ci = ::nc::TypeDB::register_type<::nc::types::ObjectClass<T>, T>(
            detail::EcsComponentTypeName<T>::value.data()
        );
        return ci;
    }

    const ::nc::types::RecordInfo& get_class_info() const
    {
        return CLASS_INFO();
    }
};

} // namespace nc

/**
 * @brief Declare an ECS component (see EcsComponentBase).
 *
 * An ECS component is typically a POD struct with REFLECT-annotated fields.
 * It is registered with the ECS world.
 *
 * Example use:
 * @code
 * NC_COMPONENT(FooBar)
 * {
 *     REFLECT int value = 0;
 * };
 * @endcode
 */
#define NC_COMPONENT( N ) struct N : ::nc::EcsComponentBase<N>

/**
 * @brief Same as NC_COMPONENT but exports the type from ncore (NCAPI).
 */
#define NC_COMPONENT_API( N ) struct NCAPI N : ::nc::EcsComponentBase<N>
