#pragma once

namespace MemoryAllocator
{
// cross platform macro that uses the right attribute per compiler to override the heuristic and force inlining
#if defined(_MSC_VER)
    #define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCE_INLINE inline [[gnu::always_inline]] inline
#else
    #define FORCE_INLINE inline
#endif


// Unit testing assert macros
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
}
