#include <atomic>

#include <ncore/core/rid.h>

namespace nc::detail {
uint32_t next_rid_sequence() noexcept
{
    static std::atomic<uint32_t> seq{ 1 }; // First RIDPool acquire will get the value 1
    return seq.fetch_add( 1, std::memory_order_relaxed );
}
} // namespace nc::detail
