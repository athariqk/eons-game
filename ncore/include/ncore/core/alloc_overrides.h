#pragma once

// ----------------------------------------------------------------------------
// Global operator new/delete overrides for NCORE.
//
// This header defines all replaceable C++ global operator new and operator
// delete variants, routing every allocation through the NCORE allocation API
// (memalloc / memfree / memalloc_aligned / memfree_align), which is backed by
// mimalloc.
//
// In AddressSanitizer builds (NC_ASAN_ENABLED) this header is a no-op: the
// default CRT operator new/delete are left in place so ASAN's allocator
// intercepts all C++ allocations (with allocation stacks for leak reports).
//
// IMPORTANT: This header must be included in exactly ONE translation unit per
// module (DLL or EXE). These are non-inline definitions; including it in
// multiple .cpp files in the same module will cause linker errors (duplicate
// symbols). Do NOT include this in a precompiled header.
//
// In ncore.dll:  memalloc/memfree resolve to the local mimalloc calls.
// In eons-game:  memalloc/memfree resolve to DLL imports from ncore.dll,
//                ensuring both modules share the same heap.
// ----------------------------------------------------------------------------

#include <new>

#include <ncore/core/memory.h>

#if !NC_ASAN_ENABLED

// ============================================================================
// Scalar new / delete
// ============================================================================

void* operator new( std::size_t n ) noexcept( false )
{
    void* p = nc::memalloc( n );
    if (!p)
        throw std::bad_alloc();
    return p;
}

void* operator new[]( std::size_t n ) noexcept( false )
{
    void* p = nc::memalloc( n );
    if (!p)
        throw std::bad_alloc();
    return p;
}

void operator delete( void* p ) noexcept
{
    nc::memfree( p );
}

void operator delete[]( void* p ) noexcept
{
    nc::memfree( p );
}

// ============================================================================
// nothrow variants
// ============================================================================

void* operator new( std::size_t n, const std::nothrow_t& ) noexcept
{
    return nc::memalloc( n );
}

void* operator new[]( std::size_t n, const std::nothrow_t& ) noexcept
{
    return nc::memalloc( n );
}

void operator delete( void* p, const std::nothrow_t& ) noexcept
{
    nc::memfree( p );
}

void operator delete[]( void* p, const std::nothrow_t& ) noexcept
{
    nc::memfree( p );
}

// ============================================================================
// sized delete (C++14)
// ============================================================================

void operator delete( void* p, std::size_t /*n*/ ) noexcept
{
    nc::memfree( p );
}

void operator delete[]( void* p, std::size_t /*n*/ ) noexcept
{
    nc::memfree( p );
}

// ============================================================================
// aligned new / delete (C++17)
// ============================================================================

void* operator new( std::size_t n, std::align_val_t al ) noexcept( false )
{
    void* p = nc::memalloc_aligned( n, static_cast<size_t>( al ) );
    if (!p)
        throw std::bad_alloc();
    return p;
}

void* operator new[]( std::size_t n, std::align_val_t al ) noexcept( false )
{
    void* p = nc::memalloc_aligned( n, static_cast<size_t>( al ) );
    if (!p)
        throw std::bad_alloc();
    return p;
}

void operator delete( void* p, std::align_val_t al ) noexcept
{
    nc::memfree_align( p, static_cast<size_t>( al ) );
}

void operator delete[]( void* p, std::align_val_t al ) noexcept
{
    nc::memfree_align( p, static_cast<size_t>( al ) );
}

void operator delete( void* p, std::size_t /*n*/, std::align_val_t al ) noexcept
{
    nc::memfree_align( p, static_cast<size_t>( al ) );
}

void operator delete[]( void* p, std::size_t /*n*/, std::align_val_t al ) noexcept
{
    nc::memfree_align( p, static_cast<size_t>( al ) );
}

void* operator new( std::size_t n, std::align_val_t al, const std::nothrow_t& ) noexcept
{
    return nc::memalloc_aligned( n, static_cast<size_t>( al ) );
}

void* operator new[]( std::size_t n, std::align_val_t al, const std::nothrow_t& ) noexcept
{
    return nc::memalloc_aligned( n, static_cast<size_t>( al ) );
}

void operator delete( void* p, std::align_val_t al, const std::nothrow_t& ) noexcept
{
    nc::memfree_align( p, static_cast<size_t>( al ) );
}

void operator delete[]( void* p, std::align_val_t al, const std::nothrow_t& ) noexcept
{
    nc::memfree_align( p, static_cast<size_t>( al ) );
}

#endif // !NC_ASAN_ENABLED
