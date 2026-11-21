#include <cstdio>
#include <cstring>

#include "Test.h"

int main(int argc, char** argv) {
    int ran = 0;
    for (const test::Case& testCase : test::getCases()) {
        if (argc > 1 && std::strcmp(argv[1], testCase.name) != 0)
            continue;
        const int failures = test::getFailures();
        testCase.run();
        std::printf("%s %s\n", test::getFailures() == failures ? "PASS" : "FAIL", testCase.name);
        ran++;
    }
    if (ran == 0)
        std::fprintf(stderr, "no test matches\n");
    return ran == 0 || test::getFailures() != 0 ? 1 : 0;
}
