#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

namespace MemoryAllocator {
    /// <summary>
    /// For this I'll be using a Free List since it is ideal for fixed-sized allocations.
    /// Because it is a singly linked list it will have O(1) speed.
    /// The manger will maintain a direct pointer to the first available block.
    /// </summary>
    class FixedSizePoolAllocator
    {
    private:
        struct FreeListNode
        {
            FreeListNode* next;
        };
        FreeListNode* m_freeListHead = nullptr;
        std::byte* m_memory = nullptr;
        std::size_t m_blockSize = 0;
        std::size_t m_memorySize = 0;

        // Helper method to set the head and all of the next nodes.
        void LinkBlocks(std::size_t blockCount);
    public:
        // Delete assignment operators to avoid doubling buffers
        FixedSizePoolAllocator(const FixedSizePoolAllocator&) = delete;
        FixedSizePoolAllocator& operator=(const FixedSizePoolAllocator&) = delete;

        /// <summary>
        /// The one and only constructor for this allocator
        /// </summary>
        /// <param name="blockSize">The size of each block (in bytes)</param>
        /// <param name="blockCount">How many blocks we want to allocate.</param>
        explicit FixedSizePoolAllocator(std::size_t blockSize, std::size_t blockCount);

        ~FixedSizePoolAllocator() { delete[] m_memory; }

        /// <summary>
        /// Allocates a single block
        /// </summary>
        /// <returns>The allocated block</returns>
        void* allocateBlock();

        /// <summary>
        /// Deallocates a single block
        /// </summary>
        void deallocateBlock(void* block);
    };
}   

