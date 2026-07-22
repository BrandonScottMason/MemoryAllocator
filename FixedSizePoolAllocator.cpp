#include "FixedSizePoolAllocator.hpp"
#include <iostream> // This is to use std::clog for error logging.

namespace MemoryAllocator 
{    
    // This blank namespace is for things that will remain purley local to this file so that it cannot conflict with other files.
    namespace 
    {
        // size_t max to avoid including <algorithm>
        const constexpr size_t max(const size_t& a, const size_t& b) { return (a < b) ? b : a; }
    }

    FixedSizePoolAllocator::FixedSizePoolAllocator(std::size_t blockSize, std::size_t blockCount)
    {
        if (blockCount == 0 || blockSize == 0) 
        {
            std::clog << "FSPA: Initialization of fixed size memory pool failed! BlockCount and blockSize cannot be zero.\n";
            return;
        }

        m_blockSize = max(blockSize, sizeof(FreeListNode));
        m_memorySize = m_blockSize * blockCount;
        m_memory = new (std::nothrow) std::byte[m_memorySize];

        if (m_memory == nullptr) 
        {
            std::clog << "FSPA: Initialization of fixed size memory pool failed!\n";
            return;
        }

        LinkBlocks(blockCount);
    }

    void FixedSizePoolAllocator::LinkBlocks(std::size_t blockCount)
    {
        m_freeListHead = reinterpret_cast<FreeListNode*>(m_memory);
        FreeListNode* currentNode = m_freeListHead;

        for (std::size_t i = 0; i < blockCount - 1; ++i)
        {
            std::byte* nextNode = reinterpret_cast<std::byte*>(currentNode) + m_blockSize;
            currentNode->next = reinterpret_cast<FreeListNode*>(nextNode);
            currentNode = currentNode->next;
        }

        currentNode->next = nullptr;
    }

    void* FixedSizePoolAllocator::allocateBlock(bool threaded)
    {
        if (m_memory == nullptr) 
        {
            std::clog << "FSPA: Fixed size memory pool is not initalized! Returning null.\n";
            return nullptr;
        }

        if (!m_freeListHead && !threaded)
        {
            return nullptr;
        }
        else if (threaded) 
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] {return m_freeListHead; });
        }

        FreeListNode* availableBlock = m_freeListHead;
        m_freeListHead = m_freeListHead->next;
        if (threaded) 
        { 
            m_cv.notify_one(); 
        }
        return availableBlock;
    }

    bool FixedSizePoolAllocator::deallocateBlock(void* block, bool threaded)
    {
        if (m_memory == nullptr || block == nullptr)
        {
            return false;
        }

        // Push the freed (but not deleted) block to the top of the free list
        FreeListNode* freedBlock = static_cast<FreeListNode*>(block);

        if (threaded)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
        }

        freedBlock->next = m_freeListHead;
        m_freeListHead = freedBlock;
        
        if (threaded)
        {
            m_cv.notify_one();
        }
        return true;
    }
}