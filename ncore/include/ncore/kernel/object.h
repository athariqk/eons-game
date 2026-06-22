#pragma once

#include <ncore/kernel/types.h>

namespace ncore {

/**
 * @brief Base class for types interfacing with the engine.
 * This provides runtime type reflection features and others.
 *
 * NOTE: always declare NCLASS macro in derived classes to properly
 * register them in the reflection system.
 */
class NObject {
public:
    virtual ~NObject() = default;

    virtual const std::string_view get_class_name() const = 0;
    virtual rfl::TypeId get_type_id() const = 0;

    virtual const rfl::RecordInfo &get_class_info() const = 0;

    bool is_a(rfl::TypeId type_id) const;

    template<typename T>
    bool is_a() const {
        return is_a(rfl::Registry::get_type_id<T>());
    }
};

} // namespace ncore

#define NCLASS(class_name, parent_class)                                                                               \
public:                                                                                                                \
    const std::string_view get_class_name() const override { return #class_name; }                                     \
    ::ncore::rfl::TypeId get_type_id() const override { return ::ncore::rfl::Registry::get_type_id<class_name>(); }    \
    const ::ncore::rfl::RecordInfo &get_class_info() const override {                                                  \
        return static_cast<const ::ncore::rfl::RecordInfo &>(::ncore::rfl::Registry::get<class_name>());               \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    static auto _nc_object_init_##class_name() -> ::ncore::rfl::RecordInfo & {                                         \
        static ::ncore::rfl::RecordInfo &ci = []() -> ::ncore::rfl::RecordInfo & {                                     \
            auto &c = ::ncore::rfl::Registry::emplace<::ncore::rfl::RecordInfo, class_name>(#class_name);              \
            c.parent_id = ::ncore::rfl::Registry::get_type_id<parent_class>();                                         \
            return c;                                                                                                  \
        }();                                                                                                           \
        return ci;                                                                                                     \
    }                                                                                                                  \
    inline static const int _nc_trig_nclass_##class_name = (_nc_object_init_##class_name(), 0);
