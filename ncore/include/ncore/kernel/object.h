#pragma once

#include "types.h"

namespace ncore {

/**
 * @brief Base class for most engine types.
 * This provides runtime type reflection features and others.
 *
 * NOTE: always declare NCLASS macro in derived classes to properly
 * register them in the reflection system.
 */
class NcObject {
public:
    virtual ~NcObject() = default;

    virtual const std::string_view get_class_name() const = 0;
    virtual rfl::TypeId get_type_id() const = 0;

    virtual const rfl::ClassInfo &get_class_info() const = 0;

    bool is_a(rfl::TypeId type_id) const;

    template<typename T>
    bool is_a() const {
        return is_a(rfl::type_id<T>());
    }
};

} // namespace ncore

#define NCLASS(class_name, parent_class)                                                                               \
public:                                                                                                                \
    const std::string_view get_class_name() const override { return #class_name; }                                     \
    ::ncore::rfl::TypeId get_type_id() const override { return ::ncore::rfl::type_id<class_name>(); }                  \
    const ::ncore::rfl::ClassInfo &get_class_info() const override {                                                   \
        return ::ncore::rfl::Registry::get<class_name>();                                                         \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    static auto _nc_object_init_##class_name() -> ::ncore::rfl::ClassInfo & {                                          \
        static ::ncore::rfl::ClassInfo _ci(#class_name, ::ncore::rfl::type_id<class_name>(), sizeof(class_name));      \
        static bool _once = false;                                                                                     \
        if (!_once) {                                                                                                  \
            _ci.parent_id = ::ncore::rfl::type_id<parent_class>();                                                     \
            ::ncore::rfl::Registry::register_class(_ci);                                                          \
            _once = true;                                                                                              \
        }                                                                                                              \
        return _ci;                                                                                                    \
    }                                                                                                                  \
    inline static const int _nc_trig_nclass_##class_name = (_nc_object_init_##class_name(), 0);
