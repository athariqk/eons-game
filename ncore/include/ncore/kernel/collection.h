#pragma once

#include <ncore/utils/assert.h>

#include "memory.h"

// This file defines template classes for managing objects in a collection/container.

namespace ncore {

/**
 * @brief PagedObjectPool is a memory pool that allocates objects of type T in pages.
 * See PagedAllocator for the underlying page allocation mechanism.
 * TODO: write clearer docstring later
 *
 * Useful reference: https://8dcc.github.io/programming/pool-allocator.html
 */
template<typename T>
class PagedObjectPool {
    struct FreeList {
        FreeList* next;
    };

    // ensures the input type is at least pointer-sized
    // so we can fit in a free list node into its allocated memory
    struct alignas(alignof(T)) Slot {
        std::byte data[std::max(sizeof(T), sizeof(FreeList))];
    };

public:
    PagedObjectPool(uint32_t page_capacity = PagedAllocator<T>::DEFAULT_PAGE_SIZE) :
        arena(page_capacity), free_list(nullptr), active_count(0)
    {}

    template<typename... Args>
    T* acquire(Args&&... args)
    {
        Slot* slot = nullptr;

        if (free_list) {
            slot      = reinterpret_cast<Slot*>(free_list);
            free_list = free_list->next;
        } else {
            slot = arena.alloc();
        }

        T* ptr = reinterpret_cast<T*>(slot);
        new (ptr) T(std::forward<Args>(args)...);
        ++active_count;
        return ptr;
    }

    void release(T* obj)
    {
        NC_ASSERT_RET(obj != nullptr, "cannot release a null object");
        auto slot = reinterpret_cast<Slot*>(obj);
        NC_ASSERT_RET(arena.is_bounded_ptr(slot), "object does not belong to this pool");

        obj->~T();
        new (obj) FreeList(free_list);
        FreeList* node = reinterpret_cast<FreeList*>(obj);
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

} // namespace ncore
