#pragma once

#include <compare>
#include <cstdint>

namespace ue5parse {

struct FGuid {
    std::uint32_t a = 0;
    std::uint32_t b = 0;
    std::uint32_t c = 0;
    std::uint32_t d = 0;

    auto operator<=>(const FGuid&) const = default;
    friend std::uint32_t GetTypeHash(const FGuid& Guid) { return ((Guid.a * 31u + Guid.b) * 31u + Guid.c) * 31u + Guid.d; }
};

}
