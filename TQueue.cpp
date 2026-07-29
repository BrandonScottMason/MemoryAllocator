#pragma once
#include <atomic>
#include "Constants.hpp"
#include "Macros.hpp"

namespace MemoryAllocator
{
    /// <summary>
    /// The original goal of this class was for practice and educational purposes but since I'm doing this
    /// I may as well make this thread-safe so that I don't have to worry about mutexes in my Unit Testing.
    /// The T stands for Threaded.
    /// </summary>
    template <typename T>
    class TQueue
    {
    private:
        alignas(64) std::atomic<T*> m_buffer;
        alignas(64) std::atomic<size_t> m_head{ 0 };
        alignas(64) std::atomic<size_t> m_tail{ 0 };
        alignas(64) std::atomic<size_t> m_capacity{ DEFAULT_QUEUE_CAPACITY };
        std::atomic_flag m_bufferLock = ATOMIC_FLAG_INIT;

        FORCE_INLINE void lockResize()
        {
            while (m_bufferLock.test_and_set(std::memory_order::acquire));
        }

        FORCE_INLINE void unlockResize()
        {
            m_bufferLock.clear(std::memory_order::release);
        }
        
        void enlargeOrShiftUnderLock(size_t currentHead, size_t currentTail)
        {
            size_t currentSize = currentTail - currentHead;
            size_t newCapacity;
            size_t currentCapacity = m_capacity.load(std::memory_order::acquire);
            if (currentSize == currentCapacity) // Need to enlarge
            {
                newCapacity = (currentCapacity == 0) ? DEFAULT_QUEUE_CAPACITY : (currentCapacity * 2);
            }
            else // Need to shift
            {
                newCapacity = (currentCapacity == 0) ? DEFAULT_QUEUE_CAPACITY : currentCapacity;
            }
            
            T* currBuffer = m_buffer.load(std::memory_order::relaxed);
            T* newBuffer = new T[newCapacity];

            [[gsl::suppress("6386", justification: "currentSize will always be < newCapacity here.")]]
            for (size_t i = 0; i < currentSize; ++i)
            {
                newBuffer[i] = currBuffer[i];
            }

            m_buffer.store(newBuffer, std::memory_order::release);
            m_head.store(0, std::memory_order::release);
            m_tail.store(currentSize, std::memory_order::release);
            m_capacity.store(newCapacity, std::memory_order::release);

            delete[] currBuffer;
        }
    public:
        TQueue(TQueue<T>&) = delete;
        TQueue(TQueue<T>&&) = delete;
        TQueue& operator=(const TQueue&) = delete;

        explicit FORCE_INLINE TQueue()
        {
            m_buffer.store(new T[m_capacity.load(std::memory_order::relaxed)], std::memory_order::relaxed);
        }

        ~TQueue()
        {
            delete[] m_buffer.load(std::memory_order::relaxed);
        }

        FORCE_INLINE size_t Size() const 
        {
            // Loading head first makes sure that tail >= head when tail is read next. This avoids a layout drift.
            size_t head = m_head.load(std::memory_order::acquire);
            return m_tail.load(std::memory_order::acquire) - head;
        }

        /// <summary>
        /// Puts the item at the tail. This is where the buffer gets enlarged or shifted.
        /// </summary>
        /// <param name="item">Item to be added to the queue.</param>
        FORCE_INLINE void Push(T item)
        {
            while (true)
            {
                size_t cap = Capacity();
                size_t tail = m_tail.load(std::memory_order::acquire);
                if (tail >= cap)
                {
                    lockResize();
                    size_t currHead = m_head.load(std::memory_order::relaxed);
                    size_t currTail = m_tail.load(std::memory_order::relaxed);
                    
                    // Double check to make sure we need to enlarge/shift
                    if (currTail >= m_capacity.load(std::memory_order::relaxed))
                    {
                        enlargeOrShiftUnderLock(currHead, currTail);
                    }
                    unlockResize();
                    continue;
                }

                if (m_tail.compare_exchange_weak(tail, tail + 1, std::memory_order::acquire, std::memory_order::relaxed))
                {
                    // Copy the memory address of the storage location into a local variable.
                    T* localBuffer = m_buffer.load(std::memory_order::acquire);

                    localBuffer[tail] = item;

                    std::atomic_thread_fence(std::memory_order::release);
                    break;
                }
            }
        }

        /// <summary>
        /// Similar to STL this will only pop and not return anything.
        /// </summary>
        FORCE_INLINE void Pop()
        {
            size_t head = m_head.load(std::memory_order::acquire);
            size_t tail = m_tail.load(std::memory_order::acquire);
            if ( head == tail )
                return;

            if (m_head.compare_exchange_weak(head, head + 1, std::memory_order::acquire, std::memory_order::relaxed))
            {
                if (m_head.load(std::memory_order::acquire) == m_tail.load(std::memory_order::acquire))
                {
                    lockResize();
                    // Double check for extra saftey!
                    if (m_head.load(std::memory_order::acquire) == m_tail.load(std::memory_order::acquire))
                    {
                        m_head.store(0, std::memory_order::release);
                        m_tail.store(0, std::memory_order::release);
                    }
                    unlockResize();
                }
            }
        }

        /// <summary>
        /// Similar to STL this only returns what's at the head without popping it.
        /// </summary>
        /// <returns>The next item to be popped.</returns>
        FORCE_INLINE bool Front(T& out)
        {
            // Enforces that we get the most up to date data
            std::atomic_thread_fence(std::memory_order::acquire);

            size_t head = m_head.load(std::memory_order::relaxed);
            
            if ( head >= m_tail.load(std::memory_order::relaxed))
                return false;

            T* localBuffer = m_buffer.load(std::memory_order::acquire);
            out = localBuffer[head];

            return true;
        }

        /// <summary>
        /// Never been a fan of the STL naming convention.
        /// "A good name is the best documentation." - Chris Zimmerman
        /// </summary>
        /// <returns>True if queue is empty. False if not.</returns>
        FORCE_INLINE bool IsEmpty() const
        {
            return m_head.load(std::memory_order::acquire) == m_tail.load(std::memory_order::acquire);
        }

        FORCE_INLINE size_t Capacity() const
        {
            return m_capacity.load(std::memory_order::acquire);
        }
    };
}