// MemoryAllocator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <stdexcept>
#include <vector>
#include "FixedSizePoolAllocator.hpp"

namespace MemoryAllocator
{
    void FixedSizePoolAllocatorUnitTests()
    {
        try
        {
            std::cout << "Starting FixedSizePoolAllocator unit tests..." << std::endl;
            std::cout << "Allocating zero..." << std::endl;
            FixedSizePoolAllocator badAllocator(0, 0);
            if (badAllocator.allocateBlock() != nullptr)
            {
                std::cerr << "Unit Test FAILED: badAllocator.allocateBlock() did not return a nullptr as expected!" << std::endl;
            }

            size_t blockSize = 100000;
            size_t blockCount = 10000;
            std::vector<std::byte*> blocks;
            FixedSizePoolAllocator allocator(blockSize, blockCount);

            std::cout << "Deallocating a null ptr..." << std::endl;
            allocator.deallocateBlock(nullptr);

            std::cout << "Immediately reusing an allocation..." << std::endl;
            std::byte* testBlock = reinterpret_cast<std::byte*>(allocator.allocateBlock());
            std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(testBlock);
            allocator.deallocateBlock(testBlock);
            testBlock = reinterpret_cast<std::byte*>(allocator.allocateBlock());
            if (ptr != reinterpret_cast<std::uintptr_t>(testBlock))
            {
                std::cerr << "Unit Test FAILED: Same memory address expected." << std::endl;
            }
            allocator.deallocateBlock(testBlock);

            std::cout << "Allocating as many blocks as we passed into the constructor..." << std::endl;
            for (int i = 0; i < blockCount; ++i)
            {
                blocks.emplace_back(reinterpret_cast<std::byte*>(allocator.allocateBlock()));
            }

            std::cout << "Deallocating that many blocks now..." << std::endl;

            for (int i = 0; i < blockCount; ++i)
            {
                std::byte* first_element = std::move(blocks.front());
                blocks.erase(blocks.begin());
                allocator.deallocateBlock(first_element);
            }

            std::cout << "FixedSizePooolAllocator unit tests passed!" << std::endl;
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "Runtime exception caught: " << e.what() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Generic Exception caught: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown Exception type caught!" << std::endl;
        }
    }

    void RunUnitTests()
    {
        std::cout << "Running Unit tests..." << std::endl;

        FixedSizePoolAllocatorUnitTests();

        std::cout << "Unit tests complete!" << std::endl;
    }
}

int main()
{
    std::cout << "Welcome to the Memory Allocator!" << std::endl;

    MemoryAllocator::RunUnitTests();
}
