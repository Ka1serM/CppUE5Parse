#include <cstring>

#include "Test.h"

// Runs every test, or the tests whose name contains the first argument.
int main(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : nullptr;
    int run = 0;
    for (const test::Case& testCase : test::getCases()) {
        if (filter != nullptr && std::strstr(testCase.name, filter) == nullptr) continue;
        int before = test::getFailures();
        testCase.run();
        std::printf("%s %s\n", test::getFailures() == before ? "ok  " : "FAIL", testCase.name);
        run++;
    }
    std::printf("%d tests, %d failed checks\n", run, test::getFailures());
    return test::getFailures() == 0 ? 0 : 1;
}
