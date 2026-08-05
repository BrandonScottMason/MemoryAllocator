#pragma once
#include <atomic>
#include <mutex>
#include "Constants.hpp"
#include "Macros.hpp"
#include "SQueue.hpp"

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
        SQueue<T> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_closed = false;
    public:
        explicit TQueue() = default;
        TQueue(TQueue<T>&) = delete;
        TQueue& operator=(const TQueue&) = delete;
        TQueue(TQueue<T>&&) = delete;
        TQueue& operator=(TQueue&&) = delete;
        ~TQueue() = default;

        FORCE_INLINE size_t Size() const 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.Size();
        }

        /// <summary>
        /// Puts the item at the tail. This is where the buffer gets enlarged or shifted.
        /// </summary>
        /// <param name="item">Item to be added to the queue.</param>
        template<class U>
        FORCE_INLINE void Push(U&& item)
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                if (m_closed)
                    return;

                m_queue.Push(std::forward<U>(item));
            }
            m_cv.notify_one();
        }

        /// <summary>
        /// Unlike the STL version or SQueue pop will both provide the front and pop that object.
        /// This is to prevent misuse.
        /// </summary>
        FORCE_INLINE bool WaitAndPop(T& out)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.IsEmpty() || m_closed; });
            
            if (m_closed && m_queue.IsEmpty())
                return false;

            out = std::move(m_queue.Front());
            m_queue.Pop();

            return true;
        }

        /// <summary>
        /// Non-blocking Pop that will return false if the queue is empty.
        /// </summary>
        /// <param name="out"></param>
        /// <returns>bool that indicates if the pop was successful.</returns>
        bool TryPop(T& out)
        {
            std::lock_guard lock(m_mutex);

            if (m_queue.IsEmpty())
                return false;

            out = std::move(m_queue.Front());
            m_queue.Pop();

            return true;
        }

        /// <summary>
        /// Wakes up any waiting threads.
        /// </summary>
        /// <returns></returns>
        FORCE_INLINE void Close()
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_closed = true;
            }

            m_cv.notify_all();
        }

        /// <summary>
        /// Never been a fan of the STL naming convention.
        /// "A good name is the best documentation." - Chris Zimmerman
        /// </summary>
        /// <returns>True if queue is empty. False if not.</returns>
        FORCE_INLINE bool IsEmpty() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.IsEmpty();
        }
    };
}