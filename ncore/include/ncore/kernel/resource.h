#pragma once

namespace ncore {

/**
 * @brief RID represents an arbitrary identifier to any objects,
 * essentially acting like a general-purpose opaque handle.
 */
struct RID {
    uint64_t value = 0;

    RID() = default;
    RID(uint64_t p_val) : value(p_val) {}
    bool is_valid() const { return value != 0; }
    bool operator==(const RID &other) const { return value == other.value; }
    bool operator!=(const RID &other) const { return !(*this == other); }
};

} // namespace ncore

namespace std {
template<>
struct hash<ncore::RID> {
    size_t operator()(const ncore::RID &rid) const noexcept { return std::hash<uint64_t>()(rid.value); }
};
} // namespace std
