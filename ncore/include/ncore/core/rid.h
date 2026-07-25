#pragma once

namespace nc {

/**
 * @brief RID represents an arbitrary identifier to any objects,
 * essentially acting like a general-purpose opaque handle.
 *
 * The default value of RID is 0, which is considered invalid.
 */
struct RID {
    uint64_t value = 0;

    RID() = default;
    RID( uint64_t p_val ) : value( p_val ) {}
    RID( const RID& p_rid ) : value( p_rid.value ) {}

    bool is_valid() const
    {
        return value != 0;
    }

    RID& operator=( const RID& p_rid )
    {
        value = p_rid.value;
        return *this;
    }

    bool operator==( const RID& other ) const
    {
        return value == other.value;
    }

    bool operator!=( const RID& other ) const
    {
        return !( *this == other );
    }

    RID operator++( int )
    {
        RID temp = *this;
        ++value;
        return temp;
    }
};

} // namespace nc

namespace std {
template<>
struct hash<nc::RID> {
    size_t operator()( const nc::RID& rid ) const noexcept
    {
        return std::hash<uint64_t>()( rid.value );
    }
};
} // namespace std
