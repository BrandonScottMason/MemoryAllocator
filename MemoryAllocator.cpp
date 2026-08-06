// MemoryAllocator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <cassert>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <thread>
#include "FixedSizePoolAllocator.hpp"
#include "TQueue.hpp"
#include "SQueue.hpp"

namespace MemoryAllocator
{
    class FixedAllocThreadTester
    {
    private:
        TQueue<std::byte*> m_blocks;
        size_t m_blockCount;
        FixedSizePoolAllocator m_allocator;
        std::atomic<bool> m_hasAllocFinished{ false };
        // Rule for writing better code: Do not rely on the user. If block count is being called without starting threads, 
        // then this class is being used wrong and thus we must assert.
        bool m_haveThreadsStarted = false;

        void AllocThreadFunc()
        {
            int allocCount = 0;
            while (allocCount < (m_blockCount / 2))
            {
                void* block = m_allocator.allocateBlock();
                if (block != nullptr)
                {
                    m_blocks.Push(reinterpret_cast<std::byte*>(block));
                    ++allocCount;
                }
            }

            m_hasAllocFinished.store(true, std::memory_order::release);
        }

        void DeallocThreadFunc() 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            while (!m_blocks.IsEmpty()) // While the Alloc thread is running OR m_blocks is not empty.
            {
                std::byte* out;
                ASSERT_IF_EQUAL(m_blocks.WaitAndPop(out), false);
                ASSERT_IF_EQUAL(m_allocator.deallocateBlock(out), false);
            }
        }
    public:
        FixedAllocThreadTester(std::size_t blockSize, std::size_t blockCount) : m_blockCount(blockCount), m_allocator(blockSize, blockCount) { }
        FixedAllocThreadTester(const FixedAllocThreadTester&) = delete;
        FixedAllocThreadTester& operator=(const FixedAllocThreadTester&) = delete;
        FixedAllocThreadTester(FixedAllocThreadTester&&) = delete;
        FixedAllocThreadTester& operator=(const FixedAllocThreadTester&&) = delete;
        ~FixedAllocThreadTester() = default;

        void StartThreads()
        {
            assert(!m_haveThreadsStarted);
            std::thread allocThread1 = std::thread(&FixedAllocThreadTester::AllocThreadFunc, this);
            std::thread allocThread2 = std::thread(&FixedAllocThreadTester::AllocThreadFunc, this);
            std::thread deallocThread = std::thread(&FixedAllocThreadTester::DeallocThreadFunc, this);
            m_haveThreadsStarted = true;
            allocThread1.join();
            allocThread2.join();
            deallocThread.join();
        }

        std::size_t BlockCount() 
        {
            assert(m_haveThreadsStarted);
            return m_blocks.Size(); 
        }
    };

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

    static void TQueueUTSequentialPushPop()
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
            if (intQueue.WaitAndPop(result))
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
            if (intQueue.WaitAndPop(reuslt))
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
            if (q.WaitAndPop(popResult))
            {
                ASSERT_IF_NOT_EQUAL(popResult, cycleCounter);
                cycleCounter++;
            }

            if (sleepTimeMs > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
            }
        }
    }

    static void TQueueUTTwoThreads()
    {
        std::cout << "Starting two threads. One producer and one consumer threads...\n";

        TQueue<int> intQueue;
        std::thread consumer(TQueueConsumer, std::ref(intQueue), 100, 10);
        std::thread producer(TQueueProducer, std::ref(intQueue), 100, 5);

        consumer.join();
        producer.join();

        ASSERT_IF_EQUAL(intQueue.IsEmpty(), false);

        std::cout << "Producer and consumer test complete!\n";
    }

    static void TQueueUTPushTryPop()
    {
        std::cout << "Starting push try pop test...\n";
        TQueue<int> q;
        q.Push(1);
        q.Push(2);
        ASSERT_IF_NOT_EQUAL(q.Size(), 2);
        int v = 0;
        bool ok = q.TryPop(v);
        ASSERT_IF_NOT_EQUAL(ok && v == 1, true);
        ok = q.TryPop(v);
        ASSERT_IF_NOT_EQUAL(ok && v == 2, true);
        ASSERT_IF_NOT_EQUAL(q.IsEmpty(), true);

        std::cout << "Push Try Pop test complete!\n";
    }

    static void TQueueUTWaitAndPopBlocksAndReceives()
    {
        std::cout << "Starting wait and pop blocks and recieves test...\n";

        TQueue<int> q;
        int result = -1;

        std::thread consumer([&]()
        {
            bool ok = q.WaitAndPop(result);
            assert(ok);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        q.Push(42);

        consumer.join();
        ASSERT_IF_NOT_EQUAL(result, 42);

        std::cout << "Wait and pop blocks and recieves test complete!\n";
    }

    static void TQueueUTCloseUnblocksWaiter()
    {
        std::cout << "Starting Close() unblocks waiter test...\n";

        TQueue<int> q;
        int out = -1;

        std::thread waiter([&]()
        {
            bool ok = q.WaitAndPop(out);
            // WaitAndPop should return false when queue is closed and empty
            ASSERT_IF_NOT_EQUAL(ok, false);
        });

        // Give the waiter time to block on wait
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        q.Close();

        waiter.join();

        std::cout << "Close() unblocks waiter test complete!\n";
    }

    static void TQueueUTMultipleProducersConsumers()
    {
        std::cout << "Starting multiple producers and consumers test...\n";

        TQueue<int> q;
        const int producers = 4;
        const int perProducer = 1000;
        const int total = producers * perProducer;

        std::atomic<int> poppedCount{ 0 };
        std::atomic<long long> sum{ 0 };

        // Consumers
        std::vector<std::thread> consumers;
        for (int c = 0; c < 2; ++c)
        {
            consumers.emplace_back([&]() 
            {
                int val;
                while (true)
                {
                    if (!q.WaitAndPop(val))
                        break;
                    poppedCount.fetch_add(1, std::memory_order_relaxed);
                    sum.fetch_add(val, std::memory_order_relaxed);
                }
            });
        }

        // Producers
        std::vector<std::thread> producersT;
        for (int p = 0; p < producers; ++p)
        {
            producersT.emplace_back([p, perProducer, &q]() 
            {
                int base = p * perProducer;
                for (int i = 0; i < perProducer; ++i)
                    q.Push(base + i);
            });
        }

        for (auto& t : producersT) t.join();

        // All producers done; wake consumers to finish after queue empties
        q.Close();

        for (auto& t : consumers) t.join();

        ASSERT_IF_NOT_EQUAL(poppedCount.load(), total);

        // verify sum of pushed values
        long long expectedSum = 0;
        for (int p = 0; p < producers; ++p)
        {
            int base = p * perProducer;
            for (int i = 0; i < perProducer; ++i)
                expectedSum += (base + i);
        }

        ASSERT_IF_NOT_EQUAL(sum.load(), expectedSum);

        std::cout << "Multiple producers and consumers test complete!\n";
    }

    /// <summary>
    /// Unit tests specifically for the FixedSizePoolAllocator class.
    /// </summary>
    void FixedSizePoolAllocatorUnitTests()
    {
        std::cout << "Starting FixedSizePoolAllocator unit tests...\n";

        FxdAllocUTDeallocNullptr();
        FxdAllocUTReuse();
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
        TQueueUTPushTryPop();
        TQueueUTWaitAndPopBlocksAndReceives();
        TQueueUTCloseUnblocksWaiter();
        TQueueUTMultipleProducersConsumers();

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
