#pragma once

#include <bit>
#include <cmath>
#include <new>
#include <utility>
#include <vector>

namespace ncore {

/**
 * @brief BumpAllocator defines a single contiguous block of memory on the heap.
 */
template<typename T>
class BumpAllocator {
public:
    BumpAllocator(size_t p_capacity) : capacity(p_capacity), size(0)
    {
        data = ::operator new(capacity * sizeof(T), std::align_val_t(alignof(T)));
    }

    ~BumpAllocator()
    {
        dealloc();
    }

    T* alloc()
    {
        if (size >= capacity)
            return nullptr;
        return &data[size++];
    }

    void dealloc()
    {
        size = 0;
        ::operator delete(data, std::align_val_t(alignof(T)));
    }

    T* operator[](size_t index)
    {
        if (index >= size)
            return nullptr;
        return &data[index];
    }

    // This does NOT free memory, just resets counter to 0
    void reset()
    {
        size = 0;
    }

    size_t get_size() const
    {
        return size;
    }
    size_t get_capacity() const
    {
        return capacity;
    }

private:
    size_t capacity;
    size_t size;
    T* data;
};

/**
 * @brief PagedAllocator defines a growable collection of elements on the heap,
 * allocated in pages (chunks) of fixed size. You may only ever allocate linearly
 * to the arena. Calling dealloc() frees all previously allocated pages.
 *
 * This is intended to be used on top of managers that handle object lifetimes.
 * Otherwise, you are responsible for calling the constructors and destructors
 * of any allocated objects.
 */
template<typename T>
class PagedAllocator {
public:
    static constexpr uint32_t DEFAULT_PAGE_SIZE = 4096; // 4KB

    PagedAllocator(size_t p_page_capacity = DEFAULT_PAGE_SIZE) : page_capacity(std::bit_ceil(p_page_capacity))
    {
        // these are for power of 2 division and modulo optimizations
        // used during new allocations to find the right page index
        // and slot index. (supposed to save up a few CPU cycles)
        page_shift = std::countr_zero(page_capacity);
        page_mask  = page_capacity - 1;

        pages.reserve(1);
    }

    ~PagedAllocator()
    {
        dealloc();
    }

    /**
     * @brief Allocates a new memory of the given T size in the arena.
     *
     * @return A typed pointer to it.
     */
    T* alloc()
    {
        uint32_t page_idx = size >> page_shift;
        uint32_t slot_idx = size & page_mask;

        if (page_idx >= get_page_count()) {
            auto chunk = static_cast<T*>(::operator new(page_capacity * sizeof(T), std::align_val_t(alignof(T))));
            pages.push_back(chunk);
        }

        T* ptr = &pages[page_idx][slot_idx];
        ++size;

        return ptr;
    }

    /**
     * @brief Deallocates all previously allocated pages in the arena.
     *
     * Generally, this is not necessary to call, as the destructor
     * will automatically free all pages.
     */
    void dealloc()
    {
        size = 0;
        for (auto page : pages) {
            ::operator delete(page, std::align_val_t(alignof(T)));
        }
        pages.clear();
    }

    T& operator[](uint32_t i)
    {
        return pages[i >> page_shift][i & page_mask];
    }

    /**
     * @brief Checks if a pointer is within the bounds of the allocated pages in the arena.
     */
    bool is_bounded_ptr(T* ptr) const
    {
        // yes we're iterating through every pages...
        for (auto page : pages) {
            if (ptr >= page && ptr < page + page_capacity) {
                return true;
            }
        }
        return false;
    }

    // This does NOT free memory, just resets counter to 0
    void reset()
    {
        size = 0;
    }

    // Returns the total number of elements allocated in the arena
    uint32_t get_size() const
    {
        return size;
    }

    // Returns the number of pages currently allocated
    uint32_t get_page_count() const
    {
        return pages.size();
    }

    // Returns the size of each page in the arena
    uint32_t get_page_capacity() const
    {
        return page_capacity;
    }

private:
    // "static" values
    uint32_t page_capacity = 0;
    uint32_t page_shift    = 0;
    uint32_t page_mask     = 0;

    // moving values/ptrs
    uint32_t size = 0;

    std::vector<T*> pages;
};

// ------------------------------------------------------------------------------
// Legacy
// ------------------------------------------------------------------------------

/**
 * @brief NoobPool defines a naive-implementation of an object pool for
 * managing a growable collection of reusable objects on the memory heap.
 */
template<typename T>
class NoobPool {
public:
    size_t alloc()
    {
        if (!free_indices.empty()) {
            size_t idx = free_indices.back();
            free_indices.pop_back();
            return idx;
        }

        size_t idx = items.size();
        items.emplace_back();
        return idx;
    }

    void dealloc(size_t idx)
    {
        if (idx < items.size()) {
            free_indices.push_back(idx);
        }
    }

    T* get(size_t idx)
    {
        if (idx < items.size()) {
            return &items[idx];
        }
        return nullptr;
    }

    const T* get(size_t idx) const
    {
        if (idx < items.size()) {
            return &items[idx];
        }
        return nullptr;
    }

    void clear()
    {
        items.clear();
        free_indices.clear();
    }

    size_t capacity() const
    {
        return items.size();
    }
    size_t size() const
    {
        return items.size() - free_indices.size();
    }

private:
    std::vector<T> items;
    std::vector<size_t> free_indices;
};

} // namespace ncore
