// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: reference.h - reference counted object handle (Ref<T>).

#pragma once

#include <atomic>
#include <type_traits>

#include "object.h"

namespace nc {

/**
 * @brief Ref is an auto-managing handle for dynamically-allocated
 * RefCounted objects.
 *
 * NOTE: It is recommended to use the Ref<T>::create() method for
 * creating new RefCounted instances, as the allocation and
 * subsequent deallocation will be guaranteed to be uniform then.
 *
 * This takes light inspiration from shared_ptr:
 * https://en.cppreference.com/cpp/memory/shared_ptr.
 * So, same semantics and best practices may apply.
 */
template<typename T, typename TAlloc>
class Ref final {
public:
    using element_type   = T;
    using allocator_type = TAlloc;

    Ref() : obj( nullptr ) {}

    Ref( std::nullptr_t ) : obj( nullptr ) {}

    /**
     * @brief Take ownership of raw pointer to a RefCounted instance.
     * If the instance passed in was allocated using an allocator
     * different from TAlloc, then it leads to undefined behavior.
     *
     * If the raw pointer is an underlying pointer owned by another Ref,
     * then it leads to undefined behavior.
     *
     * IMPORTANT: this assumes a from-one ref count as in RefCounted.
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

    /**
     * @brief Upcast only.
     */
    template<typename U>
        requires( std::derived_from<U, T> && !std::is_same_v<U, T> )
    Ref( const Ref<U>& other ) : obj( static_cast<T*>( const_cast<U*>( other.get() ) ) )
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

    operator bool() const
    {
        return obj != nullptr;
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

    static Ref share( T* p )
    {
        Ref r;
        r.obj = p;
        r.acquire();
        return r;
    }

    /**
     * @brief Give up ownership without decrementing.
     */
    T* detach() noexcept
    {
        T* p = obj;
        obj  = nullptr;
        return p;
    }

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
    template<class... TArgs>
        requires std::constructible_from<T, TArgs...>
    static Ref create( TArgs&&... args )
    {
        T* ptr = static_cast<T*>( allocator.allocate( 1 ) );

        try {
            std::construct_at( ptr, std::forward<TArgs>( args )... );
        } catch (...) {
            allocator.deallocate( ptr, 1 );
            throw;
        }

        return Ref( ptr );
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
        if constexpr (std::derived_from<T, T2>)
            return Ref<T2>( *this );
        else
            return obj && obj->template is_a<T2>() ? Ref<T2>::share( static_cast<T2*>( obj ) ) : Ref<T2>();
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
