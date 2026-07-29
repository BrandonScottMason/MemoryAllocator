#pragma once

namespace MemoryAllocator
{
    // size_t max to avoid including <algorithm>
    const constexpr size_t max(const size_t& a, const size_t& b) { return (a < b) ? b : a; }

    constexpr size_t DEFAULT_QUEUE_CAPACITY = 4;
}
