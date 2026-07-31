#pragma once

#include <bit>
#include <cmath>
#include <new>
#include <utility>
#include <vector>

#include <ncore.h>

namespace nc {

NCAPI void* memalloc( size_t size );
NCAPI void* memalloc_aligned( size_t size, size_t alignment );
NCAPI void memfree( void* ptr );
NCAPI void memfree_align( void* ptr, size_t alignment );
NCAPI void* memrealloc( void* ptr, size_t size );
NCAPI void* memcalloc( size_t count, size_t size );

/**
 * @brief STL-compatible allocator object for NCORE.
 */
template<typename T>
struct NcAllocator {
    using value_type = T;

    template<typename U>
    struct rebind {
        using other = NcAllocator<U>;
    };

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;
    using is_always_equal                        = std::true_type;

    NcAllocator() = default;

    template<typename U>
    NcAllocator( const NcAllocator<U>& )
    {}

    T* allocate( size_t n )
    {
        return static_cast<T*>( memalloc_aligned( n * sizeof( T ), alignof( T ) ) );
    }

    void deallocate( T* p, size_t )
    {
        memfree_align( p, alignof( T ) );
    }
};

template<typename T, typename U>
bool operator==( const NcAllocator<T>&, const NcAllocator<U>& )
{
    return true;
}

template<typename T, typename U>
bool operator!=( const NcAllocator<T>&, const NcAllocator<U>& )
{
    return false;
}

// ---------------------------------------------------------------------------

/**
 * @brief BumpAllocator defines a single contiguous block of memory on the heap.
 * Allocation means moving/bumping the "free" memory address pointer.
 */
template<typename T>
class BumpAllocator {
public:
    BumpAllocator( size_t p_capacity ) : capacity( p_capacity )
    {
        data = static_cast<T*>( memalloc_aligned( capacity * sizeof( T ), alignof( T ) ) );
    }

    ~BumpAllocator()
    {
        dealloc();
    }

    T* alloc()
    {
        if (head >= capacity)
            return nullptr;
        return &data[head++];
    }

    T* alloc_at( size_t position )
    {
        if (position > capacity)
            return nullptr;
        head = position;
        return &data[head];
    }

    void dealloc()
    {
        head = 0;
        memfree_align( data, alignof( T ) );
    }

    T* operator[]( size_t index )
    {
        if (index >= head)
            return nullptr;
        return &data[index];
    }
    const T* operator[]( size_t index ) const
    {
        if (index >= head)
            return nullptr;
        return &data[index];
    }

    /**
     * @brief This does NOT free memory, just resets the head pointer to 0!
     */
    void reset()
    {
        head = 0;
    }

    size_t get_head() const
    {
        return head;
    }
    void set_head( size_t position )
    {
        NC_ASSERT( position <= capacity, "Out of bounds" );
        head = position;
    }

    size_t get_capacity() const
    {
        return capacity;
    }

    const T* get_data() const
    {
        return data;
    }

private:
    size_t capacity = 0;
    size_t head     = 0;
    T* data         = nullptr;
};

/**
 * @brief PagedAllocator defines a growable collection of elements on the heap,
 * allocated in pages (chunks) of fixed size. You may only ever allocate linearly
 * to the arena. Pointers are guaranteed to be stable.
 * Calling dealloc() frees all previously allocated pages.
 *
 * This is intended to be used on top of managers that handle object lifetimes.
 * Otherwise, you are responsible for calling the constructors and destructors
 * of the allocated objects.
 */
template<typename T>
class PagedAllocator {
public:
    static constexpr uint32_t DEFAULT_PAGE_SIZE = 4096; // 4KB

    PagedAllocator( size_t p_page_capacity = DEFAULT_PAGE_SIZE ) :
        page_capacity( static_cast<uint32_t>( std::bit_ceil( p_page_capacity ) ) )
    {
        // these are for power of 2 division and modulo optimizations
        // used during new allocations to find the right page index
        // and slot index. (supposed to save up a few CPU cycles)
        page_shift = std::countr_zero( page_capacity );
        page_mask  = page_capacity - 1;

        pages.reserve( 1 );
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
        uint32_t idx = alloc_idx();
        return &pages[idx >> page_shift][idx & page_mask];
    }

    /**
     * @brief Allocates a new memory of the given T size in the arena.
     *
     * @return The index to it.
     */
    uint32_t alloc_idx()
    {
        uint32_t idx      = size;
        uint32_t page_idx = idx >> page_shift;

        if (page_idx >= get_page_count()) {
            auto chunk = static_cast<T*>( memalloc_aligned( page_capacity * sizeof( T ), alignof( T ) ) );
            pages.push_back( chunk );
        }

        size++;
        return idx;
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
            memfree_align( page, alignof( T ) );
        }
        pages.clear();
    }

    T* get( uint32_t i )
    {
        if (i >= size)
            return nullptr;
        return &pages[i >> page_shift][i & page_mask];
    }

    const T* get( uint32_t i ) const
    {
        if (i >= size)
            return nullptr;
        return &pages[i >> page_shift][i & page_mask];
    }

    T& operator[]( uint32_t i )
    {
        NC_ASSERT( i < size, "Index out of bounds" );
        return pages[i >> page_shift][i & page_mask];
    }

    /**
     * @brief Checks if a pointer is within the bounds of the allocated pages in the arena.
     */
    bool is_bounded_ptr( T* ptr ) const
    {
        // yes we're iterating through every pages...
        for (auto page : pages) {
            if (ptr >= page && ptr < page + page_capacity) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief This does NOT free memory, just resets the counter to 0!
     */
    void reset()
    {
        size = 0;
    }

    /**
     * @brief Returns the total number of elements allocated in the arena.
     */
    uint32_t get_size() const
    {
        return size;
    }

    /**
     * @brief Returns the number of pages currently allocated.
     */
    size_t get_page_count() const
    {
        return pages.size();
    }

    /**
     * @brief Returns the size of each page in the arena.
     */
    uint32_t get_page_capacity() const
    {
        return page_capacity;
    }

    template<bool IsConst>
    class PageIterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const T*, T*>;
        using reference         = std::conditional_t<IsConst, const T&, T&>;
        using allocator_type    = std::conditional_t<IsConst, const PagedAllocator, PagedAllocator>;

    private:
        allocator_type* m_alloc;
        uint32_t m_index;

        friend class PagedAllocator;
        template<bool>
        friend class PageIterator;

        PageIterator( allocator_type* alloc, uint32_t index ) : m_alloc( alloc ), m_index( index ) {}

    public:
        PageIterator() : m_alloc( nullptr ), m_index( 0 ) {}

        template<bool C = IsConst, typename = std::enable_if_t<C>>
        PageIterator( const PageIterator<false>& other ) : m_alloc( other.m_alloc ), m_index( other.m_index )
        {}

        // Element access
        reference operator*() const
        {
            return m_alloc->pages[m_index >> m_alloc->page_shift][m_index & m_alloc->page_mask];
        }

        pointer operator->() const
        {
            return &m_alloc->pages[m_index >> m_alloc->page_shift][m_index & m_alloc->page_mask];
        }

        reference operator[]( difference_type n ) const
        {
            return *( *this + n );
        }

        // Arithmetic
        PageIterator& operator++()
        {
            ++m_index;
            return *this;
        }
        PageIterator operator++( int )
        {
            PageIterator tmp = *this;
            ++m_index;
            return tmp;
        }

        PageIterator& operator--()
        {
            --m_index;
            return *this;
        }
        PageIterator operator--( int )
        {
            PageIterator tmp = *this;
            --m_index;
            return tmp;
        }

        PageIterator& operator+=( difference_type n )
        {
            m_index += n;
            return *this;
        }
        PageIterator& operator-=( difference_type n )
        {
            m_index -= n;
            return *this;
        }

        friend PageIterator operator+( PageIterator it, difference_type n )
        {
            return PageIterator( it.m_alloc, it.m_index + n );
        }
        friend PageIterator operator+( difference_type n, PageIterator it )
        {
            return PageIterator( it.m_alloc, it.m_index + n );
        }
        friend PageIterator operator-( PageIterator it, difference_type n )
        {
            return PageIterator( it.m_alloc, it.m_index - n );
        }

        friend difference_type operator-( const PageIterator& a, const PageIterator& b )
        {
            return static_cast<difference_type>( a.m_index ) - static_cast<difference_type>( b.m_index );
        }

        // Relational operators
        friend bool operator==( const PageIterator& a, const PageIterator& b )
        {
            return a.m_index == b.m_index;
        }
        friend bool operator!=( const PageIterator& a, const PageIterator& b )
        {
            return a.m_index != b.m_index;
        }
        friend bool operator<( const PageIterator& a, const PageIterator& b )
        {
            return a.m_index < b.m_index;
        }
        friend bool operator>( const PageIterator& a, const PageIterator& b )
        {
            return a.m_index > b.m_index;
        }
        friend bool operator<=( const PageIterator& a, const PageIterator& b )
        {
            return a.m_index <= b.m_index;
        }
        friend bool operator>=( const PageIterator& a, const PageIterator& b )
        {
            return a.m_index >= b.m_index;
        }
    };

    using iterator               = PageIterator<false>;
    using const_iterator         = PageIterator<true>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin()
    {
        return iterator( this, 0 );
    }
    iterator end()
    {
        return iterator( this, size );
    }

    const_iterator begin() const
    {
        return const_iterator( this, 0 );
    }
    const_iterator end() const
    {
        return const_iterator( this, size );
    }

    const_iterator cbegin() const
    {
        return const_iterator( this, 0 );
    }
    const_iterator cend() const
    {
        return const_iterator( this, size );
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator( end() );
    }
    reverse_iterator rend()
    {
        return reverse_iterator( begin() );
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator( end() );
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator( begin() );
    }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator( end() );
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator( begin() );
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

    void dealloc( size_t idx )
    {
        if (idx < items.size()) {
            free_indices.push_back( idx );
        }
    }

    T* get( size_t idx )
    {
        if (idx < items.size()) {
            return &items[idx];
        }
        return nullptr;
    }

    const T* get( size_t idx ) const
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

} // namespace nc
