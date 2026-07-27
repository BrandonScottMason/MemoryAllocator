// MemoryAllocator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <queue>
#include <stdexcept>
#include <thread>
#include <cassert>
#include "FixedSizePoolAllocator.hpp"

namespace MemoryAllocator
{
#ifndef NDEBUG
    #define ASSERT_IF_EQUAL(actual, expected) \
        if ((actual) == (expected))  { \
            std::cerr << "❌ Test Failed! Line " << __LINE__ << "\n"; \
        }
    #define ASSERT_IF_NOT_EQUAL(actual, expected) \
        if ((actual) != (expected)) { \
            std::cerr << "❌ Test Failed! Line " << __LINE__ << "\n"; \
        }
#else
    #define ASSERT_IF_EQUAL(actual, expected)
    #define ASSERT_IF_NOT_EQUAL(actual, expected)
#endif // !NDEBUG

    class FixedAllocThreadTester
    {
    private:
        std::thread m_deallocThread;
        std::thread m_allocThread;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<std::byte*> m_blocks;
        std::size_t m_blockCount;
        FixedSizePoolAllocator m_allocator;
        std::atomic<bool> m_hasAllocFinished = false;
        // Rule for writing better code: Do not rely on the user. If block count is being called without starting threads, 
        // then this class is being used wrong and thus we must assert.
        bool m_haveThreadsStarted = false;

        void AllocThreadFunc()
        {
            int allocCount = 0;
            while (allocCount < m_blockCount)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_blocks.push(reinterpret_cast<std::byte*>(m_allocator.allocateBlock(true)));
                m_cv.notify_one();
                ++allocCount;
            }

            m_hasAllocFinished.store(true);
        }

        void DeallocThreadFunc() 
        {
            std::byte* block;
            while (true) // While the Alloc thread is running OR m_blocks is not empty.
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return (!m_blocks.empty() || m_hasAllocFinished); }); // Just in case the Alloc thread has no more blocks to push.
                if (m_blocks.empty())
                {
                    return;
                }
                block = m_blocks.front();
                m_blocks.pop();
                ASSERT_IF_EQUAL(m_allocator.deallocateBlock(block), false);
            }
        }
    public:
        FixedAllocThreadTester(std::size_t blockSize, std::size_t blockCount) : m_blockCount(blockCount), m_allocator(blockSize, blockCount) { }
        FixedAllocThreadTester(const FixedAllocThreadTester&) = delete;
        FixedAllocThreadTester& operator=(const FixedAllocThreadTester&) = delete;
        FixedAllocThreadTester& operator=(const FixedAllocThreadTester&&) = delete;

        void StartThreads()
        {
            assert(!m_haveThreadsStarted);
            m_allocThread = std::thread(&FixedAllocThreadTester::AllocThreadFunc, this);
            m_deallocThread = std::thread(&FixedAllocThreadTester::DeallocThreadFunc, this);
            m_haveThreadsStarted = true;
            m_allocThread.join();
            m_deallocThread.join();
        }

        std::size_t BlockCount() 
        {
            assert(m_haveThreadsStarted);
            return m_blocks.size(); 
        }

        ~FixedAllocThreadTester()
        {
            if (m_deallocThread.joinable()) { m_deallocThread.join(); }
            if (m_allocThread.joinable()) { m_allocThread.join(); }
        }
    };

    void FxdAllocUTBadAllocator()
    {
        std::cout << "Allocating a pool of nothing...\n";
        FixedSizePoolAllocator badAllocator(0, 0);
        ASSERT_IF_NOT_EQUAL(badAllocator.allocateBlock(), nullptr);
    }

    void FxdAllocUTDeallocNullptr()
    {
        std::cout << "Deallocating a null ptr...\n";
        FixedSizePoolAllocator allocator(1, 1);
        ASSERT_IF_NOT_EQUAL(allocator.deallocateBlock(nullptr), false);
    }

    void FxdAllocUTReuse()
    {
        std::cout << "Immediately reusing an allocation...\n";
        FixedSizePoolAllocator allocator(100, 10);
        std::byte* testBlock = reinterpret_cast<std::byte*>(allocator.allocateBlock());
        std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(testBlock);
        ASSERT_IF_NOT_EQUAL(allocator.deallocateBlock(testBlock), true);
        testBlock = reinterpret_cast<std::byte*>(allocator.allocateBlock());
        ASSERT_IF_NOT_EQUAL(ptr, reinterpret_cast<std::uintptr_t>(testBlock)); // Testing if the memory address is the same
    }

    void FxdAllocUTExceedCap()
    {
        std::cout << "Allocating past the block count...\n";
        FixedSizePoolAllocator allocator(1, 1);
        allocator.allocateBlock();
        ASSERT_IF_NOT_EQUAL(allocator.allocateBlock(), nullptr);
    }

    void FxdAllocUTMoveManyBlocks()
    {
        size_t blockCount = 10000000;
        std::queue<std::byte*> blocks;
        FixedSizePoolAllocator allocator(1, blockCount);

        std::cout << "Allocating 1 million blocks...\n";
        for (int i = 0; i < blockCount; ++i)
        {
            std::byte* block = reinterpret_cast<std::byte*>(allocator.allocateBlock());
            ASSERT_IF_EQUAL(block, nullptr);
            blocks.push(block);
        }

        std::cout << "Deallocating 1 million blocks...\n";
        for (int i = 0; i < blockCount; ++i)
        {
            std::byte* first_element = std::move(blocks.front());
            blocks.pop();
            ASSERT_IF_NOT_EQUAL(allocator.deallocateBlock(first_element), true);
        }
    }

    void FxdAllocUTRaceCondition()
    {
        std::cout << "Starting thread saftey testing...\n";

        static std::mutex mtx;
        static std::condition_variable cv;
        size_t blockSize = 10;
        size_t blockCount = 10000;
        FixedAllocThreadTester threadTester(blockSize, blockCount);
        threadTester.StartThreads();
        ASSERT_IF_NOT_EQUAL(threadTester.BlockCount(), 0);

        std::cout << "Thread saftey testing complete!\n";
    }

    /// <summary>
    /// Unit tests specifically for the FixedSizePoolAllocator.
    /// </summary>
    void FixedSizePoolAllocatorUnitTests()
    {
        std::cout << "Starting FixedSizePoolAllocator unit tests...\n";

        FxdAllocUTBadAllocator();
        FxdAllocUTDeallocNullptr();
        FxdAllocUTReuse();
        FxdAllocUTExceedCap();
        FxdAllocUTMoveManyBlocks();
        FxdAllocUTRaceCondition();

        std::cout << "FixedSizePooolAllocator unit tests passed!\n";
    }

    /// <summary>
    /// This method contains unit tests for all Allocators in this project.
    /// </summary>
    void RunUnitTests()
    {
        std::cout << "Running Unit tests...\n";

        FixedSizePoolAllocatorUnitTests();

        std::cout << "Unit tests complete!\n";
    }
}

int main()
{
    std::cout << "Welcome to the Memory Allocator!\n";

    MemoryAllocator::RunUnitTests();

    std::cout << "Press any key to end program...\n";
    std::cin.get();
}
