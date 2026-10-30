#pragma once

// A minimal test runner: TEST registers a function, CHECK records a failure and goes on, REQUIRE returns from the
// test. The tests are their own target, so they may use std::string and exceptions where that is simpler.

#include <cstdio>
#include <vector>

namespace test {

struct Case {
    const char* name;
    void (*run)();
};

inline std::vector<Case>& getCases() {
    static std::vector<Case> cases;
    return cases;
}

inline int& getFailures() {
    static int failures = 0;
    return failures;
}

struct Registrar {
    Registrar(const char* name, void (*run)()) { getCases().push_back({name, run}); }
};

inline bool report(bool ok, const char* expression, const char* file, int line) {
    if (ok) return true;
    std::fprintf(stderr, "  %s:%d: CHECK(%s) failed\n", file, line, expression);
    getFailures()++;
    return false;
}

}

#define TEST(name)                                                                                                     \
    static void name();                                                                                                \
    static test::Registrar name##Registrar(#name, name);                                                               \
    static void name()

#define CHECK(expression) test::report(static_cast<bool>(expression), #expression, __FILE__, __LINE__)
#define REQUIRE(expression)                                                                                            \
    do {                                                                                                               \
        if (!CHECK(expression)) return;                                                                                \
    } while (false)
