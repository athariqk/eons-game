#include <mimalloc.h>

#include <ncore/kernel/memory.h>

namespace nc {

void* nc_alloc( size_t size )
{
    return mi_malloc( size );
}

void* nc_alloc_align( size_t size, size_t alignment )
{
    return mi_malloc_aligned( size, alignment );
}

void nc_free( void* ptr )
{
    mi_free( ptr );
}

void nc_free_align( void* ptr, size_t alignment )
{
    mi_free_aligned( ptr, alignment );
}

void* nc_realloc( void* ptr, size_t size )
{
    return mi_realloc( ptr, size );
}

} // namespace nc
