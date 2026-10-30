#pragma once

// A broken invariant is a bug in the library, not bad data, so it ends the process with the location.
#define UE5PARSE_CHECK(condition)                                                                                      \
    do {                                                                                                               \
        if (!(condition)) ::ue5parse::checkFailed(#condition, __FILE__, __LINE__);                                     \
    } while (false)

namespace ue5parse {

[[noreturn]] void checkFailed(const char* condition, const char* file, int line);

}
