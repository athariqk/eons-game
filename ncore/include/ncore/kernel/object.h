#pragma once

#include <ncore/kernel/types.h>

namespace nc {

/**
 * @brief NcObject is the base class for every object-oriented NCORE types.
 * This provides runtime type reflection features and others.
 *
 * NOTE: always declare NCLASS macro in derived classes to properly
 * register them in the reflection system.
 */
class NCORE_API NcObject {
public:
    virtual ~NcObject() = default;

    NcObject()                             = default;
    NcObject( const NcObject& )            = default;
    NcObject& operator=( const NcObject& ) = default;

    virtual const std::string_view get_class_name() const = 0;
    virtual rtti::TypeId get_type_id() const              = 0;

    virtual const rtti::RecordInfo& get_class_info() const = 0;

    bool is_a( rtti::TypeId type_id ) const;

    template<typename T>
    bool is_a() const
    {
        return is_a( rtti::Registry::get_type_id<T>() );
    }
};

} // namespace nc

#define NCLASS( class_name, parent_class )                                                                             \
public:                                                                                                                \
    const std::string_view get_class_name() const override                                                             \
    {                                                                                                                  \
        return #class_name;                                                                                            \
    }                                                                                                                  \
    ::nc::rtti::TypeId get_type_id() const override                                                                    \
    {                                                                                                                  \
        return ::nc::rtti::Registry::get_type_id<class_name>();                                                        \
    }                                                                                                                  \
    const ::nc::rtti::RecordInfo& get_class_info() const override                                                      \
    {                                                                                                                  \
        return static_cast<const ::nc::rtti::RecordInfo&>( ::nc::rtti::Registry::get<class_name>() );                  \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    inline static auto nc_object_init_##class_name() -> ::nc::rtti::RecordInfo&                                        \
    {                                                                                                                  \
        ::nc::rtti::RecordInfo& ci_##class_name = []() -> ::nc::rtti::RecordInfo& {                                    \
            auto& c     = ::nc::rtti::Registry::emplace<::nc::rtti::RecordInfo, class_name>( #class_name );            \
            c.parent_id = ::nc::rtti::Registry::get_type_id<parent_class>();                                           \
            return c;                                                                                                  \
        }();                                                                                                           \
        return ci_##class_name;                                                                                        \
    }                                                                                                                  \
    inline static const int nc_trig_nclass_##class_name = ( nc_object_init_##class_name(), 0 );
