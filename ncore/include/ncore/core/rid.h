#pragma once

#include <atomic>

#include <ncore.h>

#include "types.h"

namespace nc {

/**
 * @brief RID represents an arbitrary identifier to any objects,
 * essentially acting like a general-purpose opaque handle.
 *
 * The default value of RID is 0, which is considered invalid.
 */
struct REFLECT NCAPI RID {
    REFLECT uint64_t value = 0;

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

    operator bool()
    {
        return is_valid();
    }
    operator bool() const
    {
        return is_valid();
    }

    bool operator!() const
    {
        return !is_valid();
    }

    RID operator++( int )
    {
        RID temp = *this;
        ++value;
        return temp;
    }
};

// ---------------------------------------------------------------------------

namespace detail {
extern uint32_t next_rid_sequence() noexcept;
} // namespace detail

/**
 * @brief RIDPool is an object pool that provides an
 * RID-based interface for acquiring and releasing objects of
 * type T.
 *
 * This calls the constructor of T when acquiring an object and calls the
 * destructor of T when releasing it. Generated RIDs are guaranteed to be
 * unique process-wide.
 *
 * The internal allocator/storage is backed by a PagedAllocator.
 */
template<typename T>
class RIDPool {
    struct Slot {
        alignas( T ) std::byte data[sizeof( T )];
        uint32_t validator = 1; // a.k.a "generation"
        uint32_t next_free = UINT32_MAX;
        bool is_alive      = false;
    };

public:
    RIDPool( uint32_t page_capacity = PagedAllocator<Slot>::DEFAULT_PAGE_SIZE ) : arena( page_capacity ) {}

    ~RIDPool()
    {
        release_all();
    }

    RIDPool( const RIDPool& )            = delete;
    RIDPool& operator=( const RIDPool& ) = delete;
    RIDPool( RIDPool&& )                 = delete;
    RIDPool& operator=( RIDPool&& )      = delete;

    template<typename... Args>
    RID acquire( Args&&... args )
    {
        Slot* slot     = nullptr;
        uint32_t index = arena.get_size();

        if (free_list_head != UINT32_MAX) {
            index = free_list_head;
            slot  = arena.get( index );
            NC_ASSERT_MSG( slot, "Free list head points to an invalid slot" );
            free_list_head = slot->next_free;
        } else {
            slot = arena.alloc();
        }

        slot->next_free = UINT32_MAX;
        slot->validator = detail::next_rid_sequence();
        new ( &slot->data ) T( std::forward<Args>( args )... );
        slot->is_alive = true;
        return encode_rid( index, slot->validator );
    }

    T* get( RID handle )
    {
        if (!handle.is_valid())
            return nullptr;

        auto [index, validator] = decode_rid( handle );

        Slot* slot = arena.get( index );
        if (!slot || slot->validator != validator) {
            return nullptr;
        }

        return reinterpret_cast<T*>( &slot->data );
    }

    T* get_or_fail( RID p_handle )
    {
        T* obj = get( p_handle );
        NC_VERIFY_MSG( obj, std::format( "RID value {} does not map to a valid object.", p_handle.value ).c_str() );
        return obj;
    }

    template<typename... Args>
    T* acquire_and_get( Args&&... args )
    {
        return get( acquire( std::forward<Args>( args )... ) );
    }

    /**
     * @brief Free object from pool. Invalidating its handle.
     * @return True if succesfully freed.
     */
    bool release( RID handle )
    {
        if (!handle.is_valid())
            return false;

        auto [index, validator] = decode_rid( handle );

        Slot* slot = arena.get( index );
        if (!slot || slot->validator != validator) {
            return false; // probably already released or not owned by us
        }

        reinterpret_cast<T*>( &slot->data )->~T();
        slot->validator = 0;
        slot->next_free = free_list_head;
        slot->is_alive  = false;
        free_list_head  = index;

        return true;
    }

    void release_all()
    {
        for (uint32_t i = 0; i < arena.get_size(); i++) {
            Slot* slot = arena.get( i );
            if (slot && slot->is_alive) {
                T* obj = reinterpret_cast<T*>( &slot->data );
                obj->~T();
                slot->is_alive = false;
            }
        }
        arena.reset();
        free_list_head = UINT32_MAX;
    }

    /**
     * @brief Reset the internal arena but does not free memory nor call destructors.
     */
    void reset()
    {
        arena.reset();
        free_list_head = UINT32_MAX;
    }

    bool contains( RID handle ) const
    {
        if (!handle.is_valid())
            return false;
        auto [index, validator] = decode_rid( handle );
        const Slot* slot        = arena.get( index );
        return slot && slot->validator == validator;
    }

    size_t get_size() const
    {
        return arena.get_size();
    }

    /**
     * @brief This is unsafe as it may return released objects.
     */
    T& operator[]( RID handle )
    {
        T* it = get( handle );
        NC_ASSERT_MSG( it, "Out of bounds" );
        return *it;
    }

    using iterator       = SlotIterator<T, Slot, false>;
    using const_iterator = SlotIterator<T, Slot, true>;

    iterator begin()
    {
        return iterator( arena.begin(), arena.end() );
    }
    iterator end()
    {
        return iterator( arena.end(), arena.end() );
    }
    const_iterator begin() const
    {
        return const_iterator( arena.begin(), arena.end() );
    }
    const_iterator end() const
    {
        return const_iterator( arena.end(), arena.end() );
    }

private:
    static RID encode_rid( uint32_t index, uint32_t validator )
    {
        uint64_t val = ( static_cast<uint64_t>( validator ) << 32 ) | index;
        return RID( val );
    }

    static std::pair<uint32_t, uint32_t> decode_rid( RID handle )
    {
        uint32_t index     = static_cast<uint32_t>( handle.value & 0xFFFFFFFFu );
        uint32_t validator = static_cast<uint32_t>( handle.value >> 32 );
        return { index, validator };
    }

private:
    PagedAllocator<Slot> arena;
    uint32_t free_list_head = UINT32_MAX;
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
