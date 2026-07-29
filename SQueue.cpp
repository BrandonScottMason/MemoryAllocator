#pragma once
#include "Constants.hpp"
#include "Macros.hpp"

namespace MemoryAllocator
{
    /// <summary>
    /// Super fast version of a queue. NOT thread safe.
    /// The S stands for simple.
    /// </summary>
    template <typename T>
    class SQueue
    {
    private:
        T* m_buffer;
        size_t m_head;
        size_t m_tail;
        size_t m_capacity;

        void enlargeOrShift()
        {
            size_t newCapacity;
            if (Size() == m_capacity) // Need to enlarge
            {
                newCapacity = (m_capacity == 0) ? DEFAULT_QUEUE_CAPACITY : (m_capacity * 2);
            }
            else // Need to shift
            {
                newCapacity = (m_capacity == 0) ? DEFAULT_QUEUE_CAPACITY : m_capacity;
            }

            T* newBuffer = new T[newCapacity];

            [[gsl::suppress("6386", justification: "Size() will always be < newCapacity here.")]]
            for (size_t i = 0; i < Size(); ++i)
            {
                newBuffer[i] = m_buffer[i];
            }

            delete[] m_buffer;
            m_buffer = newBuffer;

            m_head = 0;
            m_capacity = newCapacity;
        }
    public:
        SQueue(SQueue<T>&) = delete;
        SQueue(SQueue<T>&&) = delete;
        SQueue& operator=(const SQueue&) = delete;

        explicit FORCE_INLINE SQueue() : m_capacity(0), m_head(0), m_tail(0)
        {
            enlargeOrShift();
        }

        ~SQueue()
        {
            delete[] m_buffer;
        }

        FORCE_INLINE size_t Size() const { return m_tail - m_head; }

        /// <summary>
        /// Puts the item at the tail. This is where the buffer gets enlarged or shifted.
        /// </summary>
        /// <param name="item"></param>
        FORCE_INLINE void Push(T item)
        {
            if (m_tail == m_capacity)
            {
                enlargeOrShift();
            }

            m_buffer[m_tail] = item;
            m_tail++;
        }

        /// <summary>
        /// Similar to STL this will only pop and not return anything.
        /// </summary>
        FORCE_INLINE void Pop()
        {
            if (IsEmpty())
                return;

            m_head++;

            if (m_head == m_tail)
            {
                m_head = 0;
                m_tail = 0;
            }
        }

        /// <summary>
        /// Similar to STL this only returns what's at the head without popping it.
        /// Calling this on an empty queue causes undefined behavior!
        /// </summary>
        /// <returns>The next item to be popped.</returns>
        FORCE_INLINE T& Front()
        {
            return m_buffer[m_head];
        }

        /// <summary>
        /// Never been a fan of the STL naming convention.
        /// "A good name is the best documentation." - Chris Zimmerman
        /// </summary>
        /// <returns>True if queue is empty. False if not.</returns>
        FORCE_INLINE bool IsEmpty() const
        {
            return m_head == m_tail;
        }
    };
}
