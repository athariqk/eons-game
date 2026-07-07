#pragma once

#include <MemoryAllocator.h>

#include <ncore/kernel/memory.h>

namespace nc {

struct NcoreDiligentAllocator : public Diligent::IMemoryAllocator {
    virtual ~NcoreDiligentAllocator() = default;

    void* Allocate(
        size_t Size, const Diligent::Char* /*dbgDescription*/, const char* /*dbgFileName*/,
        const Diligent::Int32 /*dbgLineNumber*/
    ) override
    {
        return memalloc( Size );
    }

    void Free( void* Ptr ) override
    {
        memfree( Ptr );
    }

    void* AllocateAligned(
        size_t Size, size_t Alignment, const Diligent::Char* /*dbgDescription*/, const char* /*dbgFileName*/,
        const Diligent::Int32 /*dbgLineNumber*/
    ) override
    {
        return memalloc_aligned( Size, Alignment );
    }

    void FreeAligned( void* Ptr ) override
    {
        // Diligent's FreeAligned does not pass the alignment back, but mimalloc
        // tracks alignment internally, so memfree (mi_free) is safe here.
        memfree( Ptr );
    }
};

} // namespace nc
