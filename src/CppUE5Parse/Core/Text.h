#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "CppUE5Parse/Core/Arena.h"

namespace ue5parse {

class StringView {
public:
    StringView() = default;
    StringView(std::span<const std::byte> bytes, bool wide) : bytes(bytes), wide(wide) {}

    bool isEmpty() const { return bytes.empty(); }
    std::string_view toUtf8(Arena& arena) const;
    std::string toString() const;

private:
    std::span<const std::byte> bytes;
    bool wide = false;
};

std::string_view utf16ToUtf8(std::span<const std::byte> utf16, Arena& arena);
std::string_view latin1ToUtf8(std::string_view latin1, Arena& arena);

}
