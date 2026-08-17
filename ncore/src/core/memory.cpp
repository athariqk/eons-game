#include <cstdint>

#if !NC_ASAN_ENABLED
#include <mimalloc.h>
#else
#include <cstdlib>
#endif

#include <ncore/core/memory.h>

namespace nc {

#if NC_ASAN_ENABLED

// ---------------------------------------------------------------------------
// AddressSanitizer builds bypass mimalloc and route every allocation through
// the CRT malloc family, which the sanitizer intercepts — giving redzones,
// use-after-free detection, and leak reports with allocation stacks.
//
// The dynamic ASAN runtime on Windows does not intercept _aligned_malloc, so
// aligned allocation is done manually: over-allocate and store the base
// pointer in a small header immediately before the aligned pointer.
// ---------------------------------------------------------------------------

namespace {

struct NcAsanAlignedHeader {
    void* base;
};

} // namespace

void* memalloc( size_t size )
{
    return malloc( size );
}

void* memalloc_aligned( size_t size, size_t alignment )
{
    const size_t al = alignment < alignof( NcAsanAlignedHeader ) ? alignof( NcAsanAlignedHeader ) : alignment;
    void* base      = malloc( size + al + sizeof( NcAsanAlignedHeader ) );
    if (!base)
        return nullptr;

    const uintptr_t raw = reinterpret_cast<uintptr_t>( base ) + sizeof( NcAsanAlignedHeader );
    void* aligned       = reinterpret_cast<void*>( ( raw + al - 1 ) & ~static_cast<uintptr_t>( al - 1 ) );
    reinterpret_cast<NcAsanAlignedHeader*>( aligned )[-1].base = base;
    return aligned;
}

void memfree( void* ptr )
{
    free( ptr );
}

void memfree_align( void* ptr, size_t /*alignment*/ )
{
    if (!ptr)
        return;
    free( reinterpret_cast<NcAsanAlignedHeader*>( ptr )[-1].base );
}

void* memrealloc( void* ptr, size_t size )
{
    return realloc( ptr, size );
}

void* memcalloc( size_t count, size_t size )
{
    return calloc( count, size );
}

#else  // !NC_ASAN_ENABLED

void* memalloc( size_t size )
{
    return mi_malloc( size );
}

void* memalloc_aligned( size_t size, size_t alignment )
{
    return mi_malloc_aligned( size, alignment );
}

void memfree( void* ptr )
{
    mi_free( ptr );
}

void memfree_align( void* ptr, size_t alignment )
{
    mi_free_aligned( ptr, alignment );
}

void* memrealloc( void* ptr, size_t size )
{
    return mi_realloc( ptr, size );
}

void* memcalloc( size_t count, size_t size )
{
    return mi_calloc( count, size );
}

#endif // NC_ASAN_ENABLED

} // namespace nc

// Global operator new/delete overrides for the ncore DLL.
// Must be included in exactly one translation unit in this module.
// This captures all C++ allocations within ncore.dll, including statically
// linked libraries (Box2D, ImGui, DiligentCore).
// (No-op in ASAN builds — see alloc_overrides.h.)
#include <ncore/core/alloc_overrides.h>
