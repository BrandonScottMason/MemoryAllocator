#pragma once
#include <atomic>
#include <mutex>
#include "Constants.hpp"
#include "Macros.hpp"
#include "SQueue.cpp"

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
        SQueue<std::shared_ptr<T>> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
    public:
        explicit TQueue() = default;
        TQueue(TQueue<T>&) = delete;
        TQueue(TQueue<T>&&) = delete;
        TQueue& operator=(const TQueue&) = delete;
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
        FORCE_INLINE void Push(T item)
        {
            std::shared_ptr<T> data = std::make_shared<T>(std::move(item));
            std::scoped_lock<std::mutex> lock(m_mutex);
            m_queue.Push(data);
            m_cv.notify_one();
        }

        /// <summary>
        /// Unlinke the STL version or SQueue pop will both provide the front and pop that object.
        /// This is to prevent misuse.
        /// </summary>
        FORCE_INLINE bool Pop(T& out)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.IsEmpty(); });
            std::shared_ptr<T> ret = m_queue.Front();
            out = *ret;
            m_queue.Pop();

            return true;
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