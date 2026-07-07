// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: Reference count mechanisms

#pragma once

#include <atomic>
#include <type_traits>

#include "memory.h"
#include "object.h"

namespace nc {

/**
 * @brief RefCounted is a typical implementation of an
 * intrusive reference counting mechanism, in this case
 * for NcObjects.
 *
 * Useful read: https://baptiste-wicht.com/posts/2011/11/boost-intrusive_ptr.html,
 *				https://chadaustin.me/2023/11/reference-counting-things/
 */
class NCORE_API RefCounted : public NcObject {
public:
    RefCounted() = default;

    RefCounted( const RefCounted& other )            = delete;
    RefCounted& operator=( const RefCounted& other ) = delete;

    void increment_ref()
    {
        ref_count.fetch_add( 1, std::memory_order_relaxed );
    }

    /**
     * @brief Decrements the reference count in an atomic way.
     *
     * @return True if decrementing to zero.
     */
    bool decrement_ref()
    {
        if (ref_count.fetch_sub( 1, std::memory_order_release ) == 1) {
            std::atomic_thread_fence( std::memory_order_acquire );
            return true;
        }

        return false;
    }

    uint32_t get_ref_count()
    {
        return ref_count.load( std::memory_order_relaxed );
    }

private:
    mutable std::atomic<uint32_t> ref_count{ 1 };
};

/**
 * @brief Ref is an auto-managing handle for RefCounted objects.
 *
 * NOTE: It is recommended to use the Ref<T>::create() method for
 * creating new RefCounted instances, as the allocation and
 * subsequent deallocation will be guaranteed to be uniform then.
 *
 * This takes light inspiration from shared_ptr:
 * https://en.cppreference.com/cpp/memory/shared_ptr.
 * So, same semantics and best practices may apply.
 */
template<typename T, typename TAlloc = NcAllocator<T>>
class NCORE_API Ref final {
public:
    Ref() : obj( nullptr ) {}

    Ref( std::nullptr_t ) : obj( nullptr ) {}

    /**
     * @brief Take ownership of raw pointer to a RefCounted instance.
     * If the instance passed in was allocated using an allocator
     * different from TAlloc, then it leads to undefined behavior.
     *
     * If the raw pointer is an underlying pointer owned by another Ref,
     * then it is leads to undefined behavior.
     *
     * IMPORTANT: This assumes a from-one ref count as in RefCounted.
     * Hence, reference count won't be increased.
     */
    explicit Ref( T* p_obj ) : obj( p_obj ) {}

    Ref( Ref&& other ) : obj( other.obj )
    {
        other.obj = nullptr;
    }

    Ref( const Ref& other ) : obj( other.obj )
    {
        acquire();
    }

    template<std::derived_from<T> U>
    Ref( const Ref<U>& other ) : obj( const_cast<U*>( other.get() ) )
    {
        acquire();
    }

    ~Ref()
    {
        release();
    }

    // ---------------------------------------------------------------------------

    Ref& operator=( const Ref& other )
    {
        if (obj == other.obj)
            return *this;

        other.acquire();
        release();
        obj = other.obj;
        return *this;
    }

    Ref& operator=( Ref&& other )
    {
        if (obj == other.obj)
            return *this;

        release();
        obj       = other.obj;
        other.obj = nullptr;
        return *this;
    }

    /**
     * @brief If the instance assigned was allocated using an
     * allocator different from TAlloc, then it's undefined
     * behavior.
     */
    Ref& operator=( T* raw )
    {
        if (obj == raw)
            return *this;

        release();
        obj = raw;
        return *this;
    }

    // ---------------------------------------------------------------------------

    T& operator*()
    {
        return *obj;
    }
    const T& operator*() const
    {
        return *obj;
    }

    T* operator->()
    {
        return obj;
    }
    const T* operator->() const
    {
        return obj;
    }

    operator bool()
    {
        return obj;
    }
    operator bool() const
    {
        return obj;
    }

    bool operator!() const
    {
        return !obj;
    }

    bool operator==( const Ref& other ) const
    {
        return obj == other.obj;
    }

    bool operator!=( const Ref& other ) const
    {
        return obj != other.obj;
    }

    // ---------------------------------------------------------------------------

    /**
     * @brief Return the raw pointer to the managed instance.
     */
    T* get()
    {
        return obj;
    }

    /**
     * @brief Return the raw pointer to the managed instance.
     */
    const T* get() const
    {
        return obj;
    }

    /**
     * @brief Release the managed instance and sets the held pointer to null.
     */
    void reset()
    {
        release();
        obj = nullptr;
    }

    /**
     * @brief Construct a new managed RefCounted instance.
     *
     * IMPORTANT: This assumes a from-one ref count as in RefCounted.
     * Hence, reference count won't be increased.
     *
     * @return A typed Ref<T> handle to it.
     */
    template<typename... TArgs>
    static Ref<T> create( TArgs&&... args )
    {
        T* ptr = static_cast<T*>( allocator.allocate( 1 ) );

        try {
            new ( ptr ) T( std::forward<TArgs>( args )... );
        } catch (...) {
            allocator.deallocate( ptr, 1 );
            throw;
        }

        return Ref<T>( ptr );
    }

    /**
     * @brief Convert the type of the managed instance to another.
     *
     * This copy-contsructs a new reference, incrementing the reference
     * count in the process.
     */
    template<std::derived_from<RefCounted> T2>
    Ref<T2> as() const
    {
        return Ref<T2>( *this );
    }

private:
    void acquire() const
    {
        if (obj)
            obj->increment_ref();
    }

    void release()
    {
        if (!obj)
            return;

        if (obj->decrement_ref()) {
            obj->~T();
            allocator.deallocate( obj, 1 );
        }
    }

    mutable T* obj;
    static inline TAlloc allocator;
};

} // namespace nc
