#pragma once

namespace MemoryAllocator
{
    constexpr size_t DEFAULT_TQUEUE_CAPACITY = 4;

    /// <summary>
    /// The original goal of this class was for practice and educational purposes but since I'm doing this
    /// I may as well make this thread-safe (TODO) so that I don't have to worry about mutexes in my Unit Testing.
    /// The T stands for Threaded (TODO).
    /// </summary>
    template <typename T>
    class TQueue
    {
    private:
        T* m_buffer;
        size_t m_head;
        size_t m_tail;
        size_t m_capacity;
        
        void enlargeOrShift()
        {
            size_t currentSize = Size();
            size_t newCapacity;
            if (currentSize == m_capacity) // Need to enlarge
            {
                newCapacity = (m_capacity == 0) ? DEFAULT_TQUEUE_CAPACITY : (m_capacity * 2);
            }
            else // Need to shift
            {
                newCapacity = (m_capacity == 0) ? DEFAULT_TQUEUE_CAPACITY : m_capacity;
            }
            
            T* newBuffer = new T[newCapacity];

            [[gsl::suppress(6386, justification: "currentSize will always be < newCapacity here.")]]
            for (size_t i = 0; i < currentSize; ++i)
            {
                newBuffer[i] = m_buffer[i];
            }

            delete[] m_buffer;
            m_buffer = newBuffer;

            m_head = 0;
            m_tail = currentSize;
            m_capacity = newCapacity;
        }
    public:
        TQueue(TQueue<T>&) = delete;
        TQueue(TQueue<T>&&) = delete;
        TQueue& operator=(const TQueue&) = delete;

        TQueue() : m_capacity(0), m_head(0), m_tail(0)
        {
            enlargeOrShift();
        }

        ~TQueue()
        {
            delete[] m_buffer;
        }

        size_t Size() { return m_tail - m_head; }

        /// <summary>
        /// Puts the item at the tail. This is where the buffer gets enlarged or shifted.
        /// </summary>
        /// <param name="item"></param>
        void Push(T item)
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
        void Pop()
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
        /// </summary>
        /// <returns>The next item to be popped.</returns>
        T Front()
        {
            return m_buffer[m_head];
        }

        /// <summary>
        /// Never been a fan of the STL naming convention.
        /// "A good name is the best documentation." - Chris Zimmerman
        /// </summary>
        /// <returns>True if queue is empty. False if not.</returns>
        bool IsEmpty()
        {
            return m_head == m_tail;
        }

    };
}