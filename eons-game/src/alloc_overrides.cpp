// Global operator new/delete overrides for the eons-game executable.
// Must be included in exactly one translation unit in this module.
// memalloc/memfree resolve to DLL imports from ncore.dll, ensuring both
// modules share the same mimalloc heap.
#include <ncore/kernel/alloc_overrides.h>
