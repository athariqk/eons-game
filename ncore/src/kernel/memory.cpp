#include <mimalloc.h>

#include <ncore/kernel/memory.h>

namespace nc {

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

} // namespace nc

// Global operator new/delete overrides for the ncore DLL.
// Must be included in exactly one translation unit in this module.
// This captures all C++ allocations within ncore.dll, including statically
// linked libraries (Box2D, ImGui, DiligentCore).
#include <ncore/kernel/alloc_overrides.h>
