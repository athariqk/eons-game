#pragma once

#include "collection.h"
#include "types.h"

namespace nc {

/**
 * @brief Object is the base class for every object-oriented NCORE types.
 * It provides first-class runtime type (RTTI) reflection features.
 *
 * NOTE: always declare NCLASS macro in derived classes to properly
 * register them in the reflection system.
 */
class NCAPI Object {
public:
    virtual ~Object() = default;

    Object()                           = default;
    Object( const Object& )            = default;
    Object& operator=( const Object& ) = default;

    virtual const StringView get_class_name() const = 0;
    virtual rtti::TypeId get_type_id() const        = 0;

    virtual const rtti::RecordInfo& get_class_info() const = 0;

    bool is_a( rtti::TypeId type_id ) const;

    template<typename T>
    bool is_a() const
    {
        return is_a( rtti::TypeRegistry::get_type_id<T>() );
    }
};

} // namespace nc

//------------------------------------------------------------------------------

#define NCLASS( class_name, parent_class )                                                                             \
public:                                                                                                                \
    const ::nc::StringView get_class_name() const override                                                             \
    {                                                                                                                  \
        return #class_name;                                                                                            \
    }                                                                                                                  \
    ::nc::rtti::TypeId get_type_id() const override                                                                    \
    {                                                                                                                  \
        return ::nc::rtti::TypeRegistry::get_type_id<class_name>();                                                    \
    }                                                                                                                  \
    const ::nc::rtti::RecordInfo& get_class_info() const override                                                      \
    {                                                                                                                  \
        return static_cast<const ::nc::rtti::RecordInfo&>( ::nc::rtti::TypeRegistry::get<class_name>() );              \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    inline static auto nc_info_##class_name() -> ::nc::rtti::RecordInfo&                                               \
    {                                                                                                                  \
        ::nc::rtti::RecordInfo& ci_##class_name = []() -> ::nc::rtti::RecordInfo& {                                    \
            auto& c = ::nc::rtti::TypeRegistry::register_type<::nc::rtti::RecordInfoT<class_name>, class_name>(        \
                #class_name                                                                                            \
            );                                                                                                         \
            c.parent_id = ::nc::rtti::TypeRegistry::get_type_id<parent_class>();                                       \
            return c;                                                                                                  \
        }();                                                                                                           \
        return ci_##class_name;                                                                                        \
    }                                                                                                                  \
    inline static const int nc_register_##class_name = ( nc_info_##class_name(), 0 );
