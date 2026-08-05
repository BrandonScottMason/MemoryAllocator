#pragma once
#include <memory>
#include "Constants.hpp"
#include "Macros.hpp"

namespace MemoryAllocator
{
    /// <summary>
    /// Circular Queue
    /// The S stands for simple.
    /// </summary>
    template <typename T>
    class SQueue
    {
    private:
        std::unique_ptr<T[]> m_buffer;
        size_t m_head;
        size_t m_tail;
        size_t m_capacity;
        size_t m_size;

        void enlarge()
        {
            static_assert(std::is_default_constructible<T>::value, "SQueue<T> requires T to be default-constructible.");
            size_t newCapacity = (m_capacity == 0) ? DEFAULT_QUEUE_CAPACITY : (m_capacity * 2);
            std::unique_ptr<T[]> newBuffer = std::make_unique<T[]>(newCapacity);

            if (m_capacity != 0 && m_size != 0)
            {
                for (size_t i = 0; i < m_size; ++i)
                {
                    newBuffer[i] = std::move(m_buffer[(m_head + i) % m_capacity]);
                }
            }

            m_buffer = std::move(newBuffer);
            m_head = 0;
            m_tail = m_size;
            m_capacity = newCapacity;
        }

        void copyFrom(const SQueue& other)
        {
            static_assert(std::is_default_constructible<T>::value, "SQueue<T> requires T to be default-constructible.");
            std::unique_ptr<T[]> newBuffer = std::make_unique<T[]>(other.m_capacity);

            for (size_t i = 0; i < other.m_size; ++i)
            {
                newBuffer[i] = other.m_buffer[(other.m_head + i) % other.m_capacity];
            }

            m_buffer = std::move(newBuffer);
            m_capacity = other.m_capacity;
            m_head = 0;
            m_tail = other.m_size;
            m_size = other.m_size;
        }

    public:
        explicit SQueue() : m_buffer(nullptr), m_capacity(0), m_head(0), m_tail(0), m_size(0)
        {
            enlarge();
        }
        
        SQueue(const SQueue<T>& other)
        {
            copyFrom(other);
        }

        SQueue& operator=(const SQueue& other)
        {
            if (this != &other)
                copyFrom(other);

            return *this;
        }

        SQueue(SQueue&& other) noexcept
            : m_buffer(std::move(other.m_buffer)),
              m_head(other.m_head),
              m_tail(other.m_tail),
              m_capacity(other.m_capacity),
              m_size(other.m_size)
        {
            other.m_head = 0;
            other.m_tail = 0;
            other.m_capacity = 0;
            other.m_size = 0;
        }

        SQueue& operator=(SQueue&& other) noexcept
        {
            if (this != &other)
            {
                m_buffer = std::move(other.m_buffer);
                m_head = other.m_head;
                m_tail = other.m_tail;
                m_capacity = other.m_capacity;
                m_size = other.m_size;

                other.m_head = 0;
                other.m_tail = 0;
                other.m_capacity = 0;
                other.m_size = 0;
            }
            return *this;
        }

        ~SQueue() = default;

        FORCE_INLINE size_t Size() const { return m_size; }

        /// <summary>
        /// Puts the item at the tail. This is where the buffer gets enlarged or shifted.
        /// </summary>
        /// <param name="item"></param>
        FORCE_INLINE void Push(const T& item)
        {
            if (m_size == m_capacity)
            {
                enlarge();
            }

            m_buffer[m_tail] = item;
            m_tail = (m_tail + 1) % m_capacity;
            ++m_size;
        }
        /// <summary>
        /// Move Push
        /// </summary>
        /// <param name="item"></param>
        /// <returns></returns>
        FORCE_INLINE void Push(T&& item)
        {
            if (m_size == m_capacity)
            {
                enlarge();
            }

            m_buffer[m_tail] = std::move(item);
            m_tail = (m_tail + 1) % m_capacity;
            ++m_size;
        }

        /// <summary>
        /// Similar to STL this will only pop and not return anything.
        /// </summary>
        FORCE_INLINE void Pop()
        {
            if (IsEmpty())
                return;

            m_buffer[m_head].~T();
            m_head = (m_head + 1) % m_capacity;
            --m_size;
        }

        /// <summary>
        /// Similar to STL this only returns what's at the head without popping it.
        /// Calling this on an empty queue causes undefined behavior!
        /// </summary>
        /// <returns>The next item to be popped.</returns>
        FORCE_INLINE const T& Front() const
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
            return m_size == 0;
        }
    };
}
