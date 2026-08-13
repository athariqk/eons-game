// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: defines template classes and some non-template ones for managing
// objects in a collection/container.

#pragma once

#include <array>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "memory.h"
#include "rid.h"

namespace nc {

using String = std::basic_string<char, std::char_traits<char>, NcAllocator<char>>;

using StringView = std::basic_string_view<char>;

template<typename T, size_t S>
using Array = std::array<T, S>;

template<typename T>
using DynamicArray = std::vector<T, NcAllocator<T>>;

template<typename T, size_t Extent = std::dynamic_extent>
using Span = std::span<T, Extent>;

template<typename TKey, typename TVal, typename THasher = std::hash<TKey>>
using HashMap = std::unordered_map<TKey, TVal, THasher, std::equal_to<TKey>, NcAllocator<std::pair<const TKey, TVal>>>;

template<typename T, typename THasher = std::hash<T>, typename TKeyEq = std::equal_to<T>>
using HashSet = std::unordered_set<T, THasher, TKeyEq, NcAllocator<T>>;

using BytesBuffer = std::vector<std::byte, NcAllocator<std::byte>>;

// ---------------------------------------------------------------------------

template<typename T, class TSlot, bool IsConst>
class SlotIterator {
public:
    using underlying = std::conditional_t<
        IsConst, typename PagedAllocator<TSlot>::const_iterator, typename PagedAllocator<TSlot>::iterator>;

    SlotIterator( underlying it, underlying end ) : it_( it ), end_( end )
    {
        skip_non_alive();
    }

    using TQ = std::conditional_t<IsConst, const T, T>;

    TQ& operator*() const
    {
        return *reinterpret_cast<TQ*>( &( *it_ ).data );
    }

    TQ* operator->() const
    {
        return reinterpret_cast<TQ*>( &( *it_ ).data );
    }

    SlotIterator& operator++()
    {
        ++it_;
        skip_non_alive();
        return *this;
    }

    bool operator==( const SlotIterator& o ) const
    {
        return it_ == o.it_;
    }
    bool operator!=( const SlotIterator& o ) const
    {
        return it_ != o.it_;
    }

private:
    void skip_non_alive()
    {
        while (it_ != end_ && !( *it_ ).is_alive)
            ++it_;
    }

    underlying it_, end_;
};

// ---------------------------------------------------------------------------

/**
 * @brief PagedPool is a memory pool that pre-allocates objects of type T
 * in fixed-size chunks. See PagedAllocator for the underlying page allocation
 * mechanism. All pooled objects incur a 1-byte validation flag overhead.
 * TODO: write clearer docstring later
 *
 * This calls the constructor of T when acquiring an object and calls the
 * destructor of T when releasing it.
 *
 * The internal allocator is backed by a PagedAllocator.
 *
 * Useful reference: https://8dcc.github.io/programming/pool-allocator.html
 */
template<typename T>
class PagedPool {
    struct FreeList {
        FreeList* next;
    };

    // ensures the input type is at least pointer-sized
    // so we can fit in a free list node into its allocated memory
    struct alignas( alignof( T ) ) Slot {
        std::byte data[std::max( sizeof( T ), sizeof( FreeList ) )];
        bool is_alive = false;
    };

public:
    PagedPool( uint32_t page_capacity = PagedAllocator<T>::DEFAULT_PAGE_SIZE ) :
        arena( page_capacity ), free_list( nullptr ), active_count( 0 )
    {}

    ~PagedPool()
    {
        release_all();
    }

    template<typename... Args>
    T* acquire( Args&&... args )
    {
        Slot* slot = nullptr;

        if (free_list) {
            slot      = reinterpret_cast<Slot*>( free_list );
            free_list = free_list->next;
        } else {
            slot = arena.alloc();
        }

        T* ptr = reinterpret_cast<T*>( slot );
        new ( ptr ) T( std::forward<Args>( args )... );
        slot->is_alive = true;
        ++active_count;
        return ptr;
    }

    void release( T* obj )
    {
        NC_FAIL_MSG_RET( obj != nullptr, "Cannot release a null object" );
        Slot* slot = reinterpret_cast<Slot*>( obj );
        NC_FAIL_MSG_RET( arena.is_bounded_ptr( slot ), "Object does not belong to this pool" );

        obj->~T();
        new ( obj ) FreeList{ free_list };
        FreeList* node = reinterpret_cast<FreeList*>( obj );
        free_list      = node;
        slot->is_alive = false;
        --active_count;
    }

    void release_all()
    {
        for (uint32_t i = 0; i < arena.get_size(); i++) {
            Slot* slot = arena.get( i );
            if (slot && slot->is_alive) {
                T* obj = reinterpret_cast<T*>( slot );
                obj->~T();
                slot->is_alive = false;
            }
        }
        arena.reset();
        free_list = nullptr;
    }

    /**
     * @brief Reset the internal arena but does not free memory nor call destructors.
     */
    void reset()
    {
        arena.reset();
        free_list = nullptr;
    }

    uint32_t get_active_count() const
    {
        return active_count;
    }

    /**
     * @brief Alias for get_active_count()
     */
    size_t size() const
    {
        return static_cast<size_t>( active_count );
    }

    uint32_t get_page_count() const
    {
        return arena.get_page_count();
    }

    /**
     * @brief This is unsafe as it may return released objects.
     */
    T& operator[]( uint32_t i )
    {
        T* it = arena.get( i );
        NC_ASSERT( it, "Out of bounds" );
        return *it;
    }

    bool is_valid( T* obj )
    {
        Slot* slot = reinterpret_cast<Slot*>( obj );
        return slot->is_alive;
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
    PagedAllocator<Slot> arena;
    FreeList* free_list;
    uint32_t active_count = 0;
};

// ---------------------------------------------------------------------------

namespace detail {
inline std::atomic<uint64_t> g_rid_sequence{ 1 }; // First ResourcePool acquire will get the value 1
inline uint64_t next_rid_sequence() noexcept
{
    return g_rid_sequence.fetch_add( 1, std::memory_order_relaxed );
}
} // namespace detail

/**
 * @brief ResourcePool is an object pool that provides an
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
class ResourcePool {
    struct Slot {
        alignas( T ) std::byte data[sizeof( T )];
        uint32_t validator = 1; // a.k.a "generation"
        uint32_t next_free = UINT32_MAX;
        bool is_alive      = false;
    };

public:
    ResourcePool( uint32_t page_capacity = PagedAllocator<Slot>::DEFAULT_PAGE_SIZE ) : arena( page_capacity ) {}

    ~ResourcePool()
    {
        release_all();
    }

    template<typename... Args>
    RID acquire( Args&&... args )
    {
        Slot* slot     = nullptr;
        uint32_t index = arena.get_size();

        if (free_list_head != UINT32_MAX) {
            index = free_list_head;
            slot  = arena.get( index );
            NC_ASSERT( slot, "Free list head points to an invalid slot" );
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

    void release( RID handle )
    {
        if (!handle.is_valid())
            return;

        auto [index, validator] = decode_rid( handle );

        Slot* slot = arena.get( index );
        if (!slot || slot->validator != validator) {
            return; // probably already released or not owned by us
        }

        reinterpret_cast<T*>( &slot->data )->~T();
        slot->validator = 0;
        slot->next_free = free_list_head;
        slot->is_alive  = false;
        free_list_head  = index;
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
        NC_ASSERT( it, "Out of bounds" );
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

/**
 * @brief RingBuffer is an implementation of overwriting-circular-buffer.
 */
template<typename T>
class RingBuffer {
public:
    static constexpr size_t DEFAULT_CAPACITY = 512; // 512 * sizeof(T) bytes

    RingBuffer( size_t p_capacity = DEFAULT_CAPACITY ) : arena( p_capacity ) {}

    void push( const T& value )
    {
        if (full()) {
            tail = ( tail + 1 ) % static_cast<uint32_t>( arena.get_capacity() );
        } else {
            size++;
        }

        size_t head = arena.get_head();
        T* write    = arena.alloc_at( head );
        head        = ( head + 1 ) % arena.get_capacity();
        arena.set_head( head );
        *write = value;
    }

    T* pop()
    {
        if (empty())
            return nullptr;

        size--;
        T* read = arena[tail];
        tail    = ( tail + 1 ) % static_cast<uint32_t>( arena.get_capacity() );
        return read;
    }

    const T* peek() const
    {
        return empty() ? nullptr : arena[tail];
    }

    bool empty() const
    {
        return size == 0;
    }

    bool full() const
    {
        return size >= arena.get_capacity();
    }

    uint32_t get_size() const
    {
        return size;
    }

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

    private:
        RingBuffer* buffer;
        size_t logical_index;

    public:
        Iterator( RingBuffer* buf, size_t index ) : buffer( buf ), logical_index( index ) {}

        reference operator*() const
        {
            size_t physical_index = ( buffer->arena.get_head() + logical_index ) % buffer->arena.get_capacity();
            return buffer->arena[physical_index];
        }

        pointer operator->()
        {
            return &( operator*() );
        }

        Iterator& operator++()
        {
            logical_index++;
            return *this;
        }

        Iterator operator++( int )
        {
            Iterator tmp = *this;
            logical_index++;
            return tmp;
        }

        friend bool operator==( const Iterator& a, const Iterator& b )
        {
            return a.buffer == b.buffer && a.logical_index == b.logical_index;
        }

        friend bool operator!=( const Iterator& a, const Iterator& b )
        {
            return !( a == b );
        }
    };

    Iterator begin()
    {
        return Iterator( this, 0 );
    }
    Iterator end()
    {
        return Iterator( this, size );
    }

private:
    BumpAllocator<T> arena;
    uint32_t tail = 0;
    uint32_t size = 0;
};

} // namespace nc
