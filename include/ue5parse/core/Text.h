#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "ue5parse/core/Arena.h"

namespace ue5parse {

// A string as serialized: ANSI (Latin-1) or UTF-16 little endian. A view, with no conversion.
class StringView {
public:
    StringView() = default;
    StringView(std::span<const std::byte> bytes, bool wide) : bytes(bytes), wide(wide) {}

    bool isWide() const { return wide; }
    bool isEmpty() const { return bytes.empty(); }
    std::span<const std::byte> getBytes() const { return bytes; }
    // The text when it is ANSI and ASCII, else empty: the common case, with no copy.
    std::string_view getAscii() const;
    // UTF-8 in the arena. Copies only when the text is not plain ASCII.
    std::string_view toUtf8(Arena& arena) const;
    // UTF-8 as an owned string.
    std::string toString() const;

private:
    std::span<const std::byte> bytes;
    bool wide = false;
};

// UTF-16 little endian (possibly unaligned) to UTF-8, in the arena.
std::string_view utf16ToUtf8(std::span<const std::byte> utf16, Arena& arena);
// Latin-1 to UTF-8, in the arena. Returns the input when it is ASCII.
std::string_view latin1ToUtf8(std::string_view latin1, Arena& arena);

}
