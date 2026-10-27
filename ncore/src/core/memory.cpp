#include <cstdint>

#if !NC_ASAN_ENABLED
#include <mimalloc.h>
#else
#include <cstdlib>
#endif

#include <ncore/core/memory.h>

#if defined( NCORE_ENABLE_TRACY )
#include <utils/profiler.h>
#endif

namespace nc {

namespace {

// Tracy memory-event emission for the nc::mem* entry points. Every first-party
// allocation path funnels through here: operator new/delete overrides
// (alloc_overrides.h), NcAllocator/Ref<T>/containers, SDL (SDL_SetMemoryFunctions),
// Flecs (os_api) and the Diligent allocator.
//
// The thread-local guard makes the hooks re-entrancy safe: should the Tracy
// emit path ever allocate through nc::mem* on the same thread, the nested
// call is not traced (the allocation itself still happens normally).
// Compiled to nothing when Tracy is disabled.

#if defined( NCORE_ENABLE_TRACY )
thread_local bool g_tracy_emit_guard = false;
#endif

inline void trace_alloc( void* ptr, size_t size )
{
#if defined( NCORE_ENABLE_TRACY )
    if (!ptr || g_tracy_emit_guard)
        return;
    g_tracy_emit_guard = true;
    NC_MEM_ALLOC( ptr, size );
    g_tracy_emit_guard = false;
#else
    ( void ) ptr;
    ( void ) size;
#endif
}

inline void trace_free( void* ptr )
{
#if defined( NCORE_ENABLE_TRACY )
    if (!ptr || g_tracy_emit_guard)
        return;
    g_tracy_emit_guard = true;
    NC_MEM_FREE( ptr );
    g_tracy_emit_guard = false;
#else
    ( void ) ptr;
#endif
}

} // namespace

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
    void* ptr = malloc( size );
    trace_alloc( ptr, size );
    return ptr;
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
    trace_alloc( aligned, size );
    return aligned;
}

void memfree( void* ptr )
{
    trace_free( ptr );
    free( ptr );
}

void memfree_align( void* ptr, size_t /*alignment*/ )
{
    if (!ptr)
        return;
    trace_free( ptr );
    free( reinterpret_cast<NcAsanAlignedHeader*>( ptr )[-1].base );
}

void* memrealloc( void* ptr, size_t size )
{
    void* result = realloc( ptr, size );
    trace_free( ptr );
    trace_alloc( result, size );
    return result;
}

void* memcalloc( size_t count, size_t size )
{
    void* ptr = calloc( count, size );
    trace_alloc( ptr, count * size );
    return ptr;
}

#else  // !NC_ASAN_ENABLED

void* memalloc( size_t size )
{
    void* ptr = mi_malloc( size );
    trace_alloc( ptr, size );
    return ptr;
}

void* memalloc_aligned( size_t size, size_t alignment )
{
    void* ptr = mi_malloc_aligned( size, alignment );
    trace_alloc( ptr, size );
    return ptr;
}

void memfree( void* ptr )
{
    trace_free( ptr );
    mi_free( ptr );
}

void memfree_align( void* ptr, size_t alignment )
{
    trace_free( ptr );
    mi_free_aligned( ptr, alignment );
}

void* memrealloc( void* ptr, size_t size )
{
    void* result = mi_realloc( ptr, size );
    trace_free( ptr );
    trace_alloc( result, size );
    return result;
}

void* memcalloc( size_t count, size_t size )
{
    void* ptr = mi_calloc( count, size );
    trace_alloc( ptr, count * size );
    return ptr;
}

#endif // NC_ASAN_ENABLED

} // namespace nc

// Global operator new/delete overrides for the ncore DLL.
// Must be included in exactly one translation unit in this module.
// This captures all C++ allocations within ncore.dll, including statically
// linked libraries (ImGui, DiligentCore).
// (No-op in ASAN builds — see alloc_overrides.h.)
#include <ncore/core/alloc_overrides.h>
