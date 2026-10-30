#pragma once

#include <cstdint>

namespace ue5parse {

struct FGuid {
    std::uint32_t a = 0;
    std::uint32_t b = 0;
    std::uint32_t c = 0;
    std::uint32_t d = 0;

    bool isZero() const { return (a | b | c | d) == 0; }
    bool operator==(const FGuid&) const = default;
};

}
