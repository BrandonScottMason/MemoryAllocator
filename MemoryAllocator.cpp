// MemoryAllocator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <cassert>
#include <iostream>
#include <queue>
#include <random>
#include <stdexcept>
#include <thread>
#include "FixedSizePoolAllocator.hpp"
#include "TQueue.cpp"
#include "SQueue.cpp"

namespace MemoryAllocator
{
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
        std::atomic<bool> m_hasAllocFinished{ false };
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
                // Yielding here and at the start of the dealloc loop to ensure there's a back-and-fourth exhange between the two.
                // This is for testing purposes only. This back-and-fourth is inefficent. The OS's overhead, thread scheduling and
                // the fact that allocation caches are inheriently faster means this would run faster if this alloc method didn't yield
                // and is allowed to complete all cycles before the dealloc thread gets a chance to wake up from waiting.
                std::this_thread::yield();
            }

            m_hasAllocFinished.store(true, std::memory_order::release);
        }

        void DeallocThreadFunc() 
        {
            std::byte* block;
            while (true) // While the Alloc thread is running OR m_blocks is not empty.
            {
                std::this_thread::yield();
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return (!m_blocks.empty() || m_hasAllocFinished); }); // Just in case the Alloc thread has no more blocks to push.
                if (m_blocks.empty() && m_hasAllocFinished.load(std::memory_order::acquire))
                {
                    return;
                }

                ASSERT_IF_EQUAL(m_allocator.deallocateBlock(m_blocks.front(), true), false);
                m_blocks.pop();
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
        SQueue<std::byte*> blocks;
        FixedSizePoolAllocator allocator(1, blockCount);

        std::cout << "Allocating 1 million blocks...\n";
        for (int i = 0; i < blockCount; ++i)
        {
            std::byte* block = reinterpret_cast<std::byte*>(allocator.allocateBlock());
            ASSERT_IF_EQUAL(block, nullptr);
            blocks.Push(block);
        }

        std::cout << "Deallocating 1 million blocks...\n";
        for (int i = 0; i < blockCount; ++i)
        {
            std::byte* first_element = blocks.Front();
            blocks.Pop();
            ASSERT_IF_NOT_EQUAL(allocator.deallocateBlock(first_element), true);
        }
    }

    void FxdAllocUTRaceCondition()
    {
        std::cout << "Starting thread saftey testing...\n";

        static std::mutex mtx;
        static std::condition_variable cv;
        size_t blockSize = 10;
        size_t blockCount = 100000;
        FixedAllocThreadTester threadTester(blockSize, blockCount);
        threadTester.StartThreads();
        size_t endCount = threadTester.BlockCount();
        ASSERT_IF_NOT_EQUAL(endCount, 0);

        std::cout << "Thread saftey testing complete!\n";
    }

    void SQueueUTSequentialPushPop()
    {
        std::cout << "Starting a sequential push pop test...\n";

        int cycles = 10;
        SQueue<int> intQueue;

        for (int i = 0; i < (cycles / 2); i++)
        {
            intQueue.Push(i);
        }

        for(int i = 0; i < (cycles / 2); i++)
        {
            ASSERT_IF_NOT_EQUAL(intQueue.Front(), i);
            intQueue.Pop();
        }

        for (int i = 0; i < cycles; i++)
        {
            intQueue.Push(i);
        }

        for (int i = 0; i < cycles; i++)
        {
            ASSERT_IF_NOT_EQUAL(intQueue.Front(), i);
            intQueue.Pop();
        }

        ASSERT_IF_EQUAL(intQueue.IsEmpty(), false);

        std::cout << "Sequential push pop test complete!\n";
    }

    void TQueueUTSequentialPushPop()
    {
        std::cout << "Starting a sequential push pop test...\n";

        int cycles = 10;
        TQueue<int> intQueue;

        for (int i = 0; i < (cycles / 2); i++)
        {
            intQueue.Push(i);
        }

        for (int i = 0; i < (cycles / 2); i++)
        {
            int result;
            if (intQueue.Pop(result))
            {
                ASSERT_IF_NOT_EQUAL(result, i);;
            }
            else
            {
                ASSERT_IF_EQUAL(intQueue.IsEmpty(), false);
            }
        }

        for (int i = 0; i < cycles; i++)
        {
            intQueue.Push(i);
        }

        for (int i = 0; i < cycles; i++)
        {
            int reuslt;
            if (intQueue.Pop(reuslt))
            {
                ASSERT_IF_NOT_EQUAL(reuslt, i);
            }
            else
            {
                ASSERT_IF_EQUAL(intQueue.IsEmpty(), false);
            }
        }

        std::cout << "Sequential push pop test complete!\n";
    }

    void TQueueProducer(TQueue<int>& q, int cycles, int sleepTimeMs = 0)
    {
        for (int i = 0; i < cycles; i++)
        {
            q.Push(i);

            if (sleepTimeMs > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
            }
        }
    }

    void TQueueConsumer(TQueue<int>& q, int cycles, int sleepTimeMs = 0)
    {
        int cycleCounter = 0;
        while (cycleCounter < cycles)
        {
            int popResult;
            if (q.Pop(popResult))
            {
                //ASSERT_IF_NOT_EQUAL(popResult, cycleCounter);
                cycleCounter++;
            }

            if (sleepTimeMs > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
            }
        }
    }

    void TQueueUTTwoThreads()
    {
        std::cout << "Starting two threads. One producer and one consumer threads...\n";

        TQueue<int> intQueue;
        std::thread producer(TQueueProducer, std::ref(intQueue), 100, 0);
        std::thread consumer(TQueueConsumer, std::ref(intQueue), 100, 0);

        producer.join();
        consumer.join();

        ASSERT_IF_EQUAL(intQueue.IsEmpty(), false);

        std::vector<int> vec;
        while (!intQueue.IsEmpty())
        {
            int num;
            if (intQueue.Pop(num))
                vec.push_back(num);
        }

        std::cout << "Producer and consumer test complete!\n";
    }

    /// <summary>
    /// Unit tests specifically for the FixedSizePoolAllocator class.
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

        std::cout << "FixedSizePooolAllocator unit tests completed!\n";
    }

    /// <summary>
    /// Unit tests specifically for the SQueue class.
    /// </summary>
    void SQueueUnitTests()
    {
        std::cout << "Starting SQueue unit tests...\n";

        SQueueUTSequentialPushPop();

        std::cout << "SQueue unit tests completed!\n";
    }

    void TQueueUnitTests()
    {
        std::cout << "Starting TQueue unit tests...\n";

        TQueueUTSequentialPushPop();
        TQueueUTTwoThreads();

        std::cout << "TQueue unit tests completed!\n";
    }

    /// <summary>
    /// This method contains unit tests for all Allocators in this project.
    /// </summary>
    void RunUnitTests()
    {
        std::cout << "Running Unit tests...\n";

        SQueueUnitTests();
        TQueueUnitTests();
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
