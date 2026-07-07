// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: defines template classes for managing objects in a collection/container.

#pragma once

#include <set>
#include <unordered_map>

#include "memory.h"
#include "rid.h"

namespace nc {

template<typename T, size_t S>
using Array = std::array<T, S>;

template<typename T>
using Vector = std::vector<T, NcAllocator<T>>;

template<typename TKey, typename TVal>
using UnorderedMap =
    std::unordered_map<TKey, TVal, std::hash<TKey>, std::equal_to<TKey>, NcAllocator<std::pair<const TKey, TVal>>>;

using BytesBuffer = std::vector<std::byte, NcAllocator<std::byte>>;

/**
 * @brief PagedPool is a memory pool that pre-allocates objects of type T
 * in fixed-size chunks. See PagedAllocator for the underlying page allocation
 * mechanism. TODO: write clearer docstring later
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
    };

public:
    PagedPool( uint32_t page_capacity = PagedAllocator<T>::DEFAULT_PAGE_SIZE ) :
        arena( page_capacity ), free_list( nullptr ), active_count( 0 )
    {}

    ~PagedPool()
    {
        // collect all pointers to the already freed slots
        // so we don't call the destructor on them again
        std::set<void*> freed;
        while (free_list) {
            freed.insert( static_cast<void*>( free_list ) );
            free_list = free_list->next;
        }
        for (uint32_t i = 0; i < arena.get_size(); i++) {
            Slot* slot = arena.get( i );
            if (slot && !freed.contains( static_cast<void*>( slot ) )) {
                T* obj = reinterpret_cast<T*>( slot );
                obj->~T();
            }
        }
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
        ++active_count;
        return ptr;
    }

    void release( T* obj )
    {
        NC_ASSERT_RET( obj != nullptr, "Cannot release a null object" );
        auto slot = reinterpret_cast<Slot*>( obj );
        NC_ASSERT_RET( arena.is_bounded_ptr( slot ), "Object does not belong to this pool" );

        obj->~T();
        new ( obj ) FreeList{ free_list };
        FreeList* node = reinterpret_cast<FreeList*>( obj );
        free_list      = node;

        --active_count;
    }

    uint32_t get_active_count() const
    {
        return active_count;
    }

    uint32_t get_page_count() const
    {
        return arena.get_page_count();
    }

private:
    PagedAllocator<Slot> arena;
    FreeList* free_list;
    uint32_t active_count = 0;
};

/**
 * @brief PagedResourcePool is an object pool that provides an
 * RID-based interface for acquiring and releasing objects of
 * type T.
 *
 * This calls the constructor of T when acquiring an object and calls the
 * destructor of T when releasing it.
 *
 * The internal allocator is backed by a PagedAllocator.
 */
template<typename T>
class PagedResourcePool {
    struct Slot {
        alignas( T ) std::byte data[sizeof( T )];
        uint32_t generation = 1;
        uint32_t next_free  = UINT32_MAX;
    };

public:
    PagedResourcePool( uint32_t page_capacity = PagedAllocator<Slot>::DEFAULT_PAGE_SIZE ) : arena( page_capacity ) {}

    ~PagedResourcePool()
    {
        // first, collect all the already freed slots (ones that are in the free list)
        std::vector<bool> freed( arena.get_size(), false );
        for (uint32_t i = free_list_head; i != UINT32_MAX;) {
            Slot* s  = arena.get( i );
            freed[i] = true;
            i        = s->next_free;
        }
        // then, call the destructor of all the slots excluding freed
        for (uint32_t i = 0; i < arena.get_size(); i++) {
            if (!freed[i]) {
                T* obj = reinterpret_cast<T*>( &arena.get( i )->data );
                obj->~T();
            }
        }
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
        new ( &slot->data ) T( std::forward<Args>( args )... );
        return encode_rid( index, slot->generation );
    }

    T* get( RID handle )
    {
        if (!handle.is_valid())
            return nullptr;

        auto [index, generation] = decode_rid( handle );

        Slot* slot = arena.get( index );
        if (!slot || slot->generation != generation) {
            return nullptr;
        }

        return reinterpret_cast<T*>( &slot->data );
    }

    T* get( RID handle ) const
    {
        return const_cast<PagedResourcePool*>( this )->get( handle );
    }

    void release( RID handle )
    {
        if (!handle.is_valid())
            return;

        auto [index, generation] = decode_rid( handle );

        Slot* slot = arena.get( index );
        if (!slot || slot->generation != generation) {
            return; // probably already released
        }

        reinterpret_cast<T*>( &slot->data )->~T();
        slot->generation++;
        slot->next_free = free_list_head;
        free_list_head  = index;
    }

    size_t get_size() const
    {
        return arena.get_size();
    }

private:
    static RID encode_rid( uint32_t index, uint32_t generation )
    {
        uint64_t val = ( static_cast<uint64_t>( generation ) << 32 ) | index;
        return RID{ val };
    }

    static std::pair<uint32_t, uint32_t> decode_rid( RID handle )
    {
        uint32_t index      = static_cast<uint32_t>( handle.value & 0xFFFFFFFFu );
        uint32_t generation = static_cast<uint32_t>( handle.value >> 32 );
        return { index, generation };
    }

    bool is_valid( RID handle ) const
    {
        if (!handle.is_valid())
            return false;
        auto [index, generation] = decode_rid( handle );
        Slot* slot               = arena.get( index );
        return slot && slot->generation == generation;
    }

private:
    PagedAllocator<Slot> arena;
    uint32_t free_list_head = UINT32_MAX;
};

} // namespace nc
