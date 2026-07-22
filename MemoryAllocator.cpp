// MemoryAllocator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <stdexcept>
#include <queue>
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

    void FixedAllocUTbadAllocator()
    {
        std::cout << "Allocating a pool of nothing...\n";
        FixedSizePoolAllocator badAllocator(0, 0);
        ASSERT_IF_NOT_EQUAL(badAllocator.allocateBlock(), nullptr);
    }

    void FixedAllocUTdeallocNull()
    {
        std::cout << "Deallocating a null ptr...\n";
        FixedSizePoolAllocator allocator(1, 1);
        ASSERT_IF_NOT_EQUAL(allocator.deallocateBlock(nullptr), false);
    }

    void FixedAllocUTreuse()
    {
        std::cout << "Immediately reusing an allocation...\n";
        FixedSizePoolAllocator allocator(100, 10);
        std::byte* testBlock = reinterpret_cast<std::byte*>(allocator.allocateBlock());
        std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(testBlock);
        ASSERT_IF_NOT_EQUAL(allocator.deallocateBlock(testBlock), true);
        testBlock = reinterpret_cast<std::byte*>(allocator.allocateBlock());
        ASSERT_IF_NOT_EQUAL(ptr, reinterpret_cast<std::uintptr_t>(testBlock)); // Testing if the memory address is the same
    }

    void FixedAllocUTexceedCapacity()
    {
        std::cout << "Allocating past the block count...\n";
        FixedSizePoolAllocator allocator(1, 1);
        allocator.allocateBlock();
        ASSERT_IF_NOT_EQUAL(allocator.allocateBlock(), nullptr);
    }

    void FixedAllocUTmoveManyBlocks()
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

    void FixedSizePoolAllocatorUnitTests()
    {
        std::cout << "Starting FixedSizePoolAllocator unit tests...\n";

        FixedAllocUTbadAllocator();
        FixedAllocUTdeallocNull();
        FixedAllocUTreuse();
        FixedAllocUTexceedCapacity();
        FixedAllocUTmoveManyBlocks();

        std::cout << "FixedSizePooolAllocator unit tests passed!\n";
    }

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
}
