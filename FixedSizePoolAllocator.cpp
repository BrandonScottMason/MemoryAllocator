#include <algorithm>
#include "FixedSizePoolAllocator.hpp"


namespace MemoryAllocator 
{    
    FixedSizePoolAllocator::FixedSizePoolAllocator(std::size_t blockSize, std::size_t blockCount)
    {
        if (blockCount == 0)
            throw std::invalid_argument("blockCount must be > 0");

        if (blockSize == 0)
            throw std::invalid_argument("blockSize must be > 0");

        m_blockSize = std::max(blockSize, sizeof(FreeListNode));

        if (blockCount > std::numeric_limits<size_t>::max() / blockSize)
        {
            throw std::overflow_error("Pool size too large!");
        }

        m_memorySize = m_blockSize * blockCount;
        m_pool = std::make_unique<std::byte[]>(m_memorySize);
        LinkBlocks(blockCount);
    }

    void FixedSizePoolAllocator::LinkBlocks(std::size_t blockCount)
    {
        m_freeListHead = reinterpret_cast<FreeListNode*>(m_pool.get());
        FreeListNode* currentNode = m_freeListHead;

        for (std::size_t i = 0; i < blockCount - 1; ++i)
        {
            std::byte* nextNode = reinterpret_cast<std::byte*>(currentNode) + m_blockSize;
            currentNode->next = reinterpret_cast<FreeListNode*>(nextNode);
            currentNode = currentNode->next;
        }

        currentNode->next = nullptr;
    }

    void* FixedSizePoolAllocator::allocateBlock()
    {
        if (m_pool == nullptr) 
            return nullptr;

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return (m_freeListHead != nullptr); });
        FreeListNode* allocatedBlock = m_freeListHead;
        m_freeListHead = m_freeListHead->next;

        return allocatedBlock;
    }

    bool FixedSizePoolAllocator::deallocateBlock(void* block)
    {
        if (m_pool == nullptr || block == nullptr)
            return false;

        // Push the freed (but not deleted) block to the top of the free list
        FreeListNode* freedBlock = static_cast<FreeListNode*>(block);
        std::lock_guard<std::mutex> lock(m_mutex);
        freedBlock->next = m_freeListHead;
        m_freeListHead = freedBlock;
        m_cv.notify_one();

        return true;
    }
}