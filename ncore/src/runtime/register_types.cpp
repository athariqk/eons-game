#include "register_types.h"

#include <kernel/types.h>

void register_kernel_types() {
    // Force static init of fundamental TypeInfo objects
    ncore::rfl::get_type<bool>();
    ncore::rfl::get_type<int32_t>();
    ncore::rfl::get_type<uint32_t>();
    ncore::rfl::get_type<int64_t>();
    ncore::rfl::get_type<uint64_t>();
    ncore::rfl::get_type<float>();
    ncore::rfl::get_type<double>();
    ncore::rfl::get_type<size_t>();
    ncore::rfl::get_type<uint8_t>();
    ncore::rfl::get_type<std::string>();
    ncore::rfl::get_class<std::vector<int32_t>>();
}

void unregister_kernel_types() {}
