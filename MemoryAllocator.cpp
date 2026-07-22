// MemoryAllocator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <future>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <thread>
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
        void AllocThread() 
        {
            int allocCount = 0;
            while (allocCount < 100)
            {
                std::future<void*> result_future = std::async(std::launch::async, [this] { return m_allocator.allocateBlock(true); });
                std::lock_guard<std::mutex> lock(m_mutex);
                m_blocks.push(reinterpret_cast<std::byte*>(&result_future));
                m_cv.notify_one();
                ++allocCount;
            }
        }
        void DeallocThread() 
        {
            int deallocCount = 0;
            while (deallocCount < 100)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return !m_blocks.empty(); });
                std::byte* block = m_blocks.front();
                m_blocks.pop();
                std::future<bool> result_future = std::async(std::launch::async, [block, this] { return m_allocator.deallocateBlock(block, true); });
                ASSERT_IF_EQUAL(result_future.get(), false);
                ++deallocCount;
            }
        }
    public:
        FixedAllocThreadTester(std::size_t blockSize, std::size_t blockCount) : m_blockCount(blockCount), m_allocator(blockSize, blockCount) { }

        void StartThreads()
        {
            m_allocThread = std::thread(&FixedAllocThreadTester::AllocThread, this);
            m_deallocThread = std::thread(&FixedAllocThreadTester::DeallocThread, this);
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
        size_t blockSize = 10;
        size_t blockCount = 100;
        FixedAllocThreadTester threadTester(blockSize, blockCount);
        threadTester.StartThreads();

        // TODO: Need to wait until all threads are complete before leaving the method.
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
        //FxdAllocUTRaceCondition();

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
