#pragma once
#include <iostream>
#include <string>

static int _pass = 0, _fail = 0;

#define ASSERT_EQ(actual, expected, msg)                              \
    if ((actual) == (expected)) {                                     \
        std::cout << "  [PASS] " << (msg) << "\n"; _pass++;          \
    } else {                                                          \
        std::cout << "  [FAIL] " << (msg)                            \
                  << "  (got " << (actual)                           \
                  << ", expected " << (expected) << ")\n"; _fail++;  \
    }

#define ASSERT_NE(actual, expected, msg)                              \
    if ((actual) != (expected)) {                                     \
        std::cout << "  [PASS] " << (msg) << "\n"; _pass++;          \
    } else {                                                          \
        std::cout << "  [FAIL] " << (msg)                            \
                  << "  (got " << (actual)                           \
                  << ", expected not " << (expected) << ")\n"; _fail++;  \
    }

#define ASSERT_TRUE(cond, msg)  ASSERT_EQ((cond), true, (msg))
#define ASSERT_FALSE(cond, msg) ASSERT_EQ((cond), false, (msg))

#define TEST_SUMMARY()                                                 \
    std::cout << "\n结果: " << _pass << " 通过 / "                    \
              << _fail << " 失败\n";                                   \
    return (_fail > 0) ? 1 : 0;
