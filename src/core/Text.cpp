#include <cstdint>

#include "ue5parse/core/Text.h"

namespace ue5parse {

namespace {

bool isAscii(std::span<const std::byte> bytes) {
    for (std::byte byte : bytes)
        if ((static_cast<std::uint8_t>(byte) & 0x80) != 0) return false;
    return true;
}

std::size_t appendUtf8(char* out, std::uint32_t codePoint) {
    if (codePoint < 0x80) {
        out[0] = static_cast<char>(codePoint);
        return 1;
    }
    if (codePoint < 0x800) {
        out[0] = static_cast<char>(0xC0 | (codePoint >> 6));
        out[1] = static_cast<char>(0x80 | (codePoint & 0x3F));
        return 2;
    }
    if (codePoint < 0x10000) {
        out[0] = static_cast<char>(0xE0 | (codePoint >> 12));
        out[1] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (codePoint & 0x3F));
        return 3;
    }
    out[0] = static_cast<char>(0xF0 | (codePoint >> 18));
    out[1] = static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
    out[2] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
    out[3] = static_cast<char>(0x80 | (codePoint & 0x3F));
    return 4;
}

}

std::string_view StringView::getAscii() const {
    if (wide || !isAscii(bytes)) return {};
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::string_view StringView::toUtf8(Arena& arena) const {
    if (wide) return utf16ToUtf8(bytes, arena);
    return latin1ToUtf8({reinterpret_cast<const char*>(bytes.data()), bytes.size()}, arena);
}

std::string_view utf16ToUtf8(std::span<const std::byte> utf16, Arena& arena) {
    std::size_t units = utf16.size() / 2;
    // At most 3 UTF-8 bytes for each UTF-16 unit (a surrogate pair of 2 units gives 4 bytes).
    std::span<char> out = arena.allocate<char>(units * 3);
    std::size_t size = 0;
    for (std::size_t i = 0; i < units; i++) {
        std::uint32_t unit = static_cast<std::uint8_t>(utf16[i * 2]) | (static_cast<std::uint8_t>(utf16[i * 2 + 1]) << 8);
        bool high = unit >= 0xD800 && unit <= 0xDBFF;
        if (high && i + 1 < units) {
            std::uint32_t next =
                static_cast<std::uint8_t>(utf16[i * 2 + 2]) | (static_cast<std::uint8_t>(utf16[i * 2 + 3]) << 8);
            if (next >= 0xDC00 && next <= 0xDFFF) {
                unit = 0x10000 + ((unit - 0xD800) << 10) + (next - 0xDC00);
                i++;
            }
        }
        size += appendUtf8(out.data() + size, unit);
    }
    return {out.data(), size};
}

std::string_view latin1ToUtf8(std::string_view latin1, Arena& arena) {
    if (isAscii(std::as_bytes(std::span(latin1)))) return latin1;
    std::span<char> out = arena.allocate<char>(latin1.size() * 2);
    std::size_t size = 0;
    for (char c : latin1) size += appendUtf8(out.data() + size, static_cast<std::uint8_t>(c));
    return {out.data(), size};
}

std::string StringView::toString() const {
    std::string out;
    if (!wide) {
        std::string_view latin1{reinterpret_cast<const char*>(bytes.data()), bytes.size()};
        if (isAscii(bytes)) return std::string(latin1);
        out.resize(latin1.size() * 2);
        std::size_t size = 0;
        for (char c : latin1) size += appendUtf8(out.data() + size, static_cast<std::uint8_t>(c));
        out.resize(size);
        return out;
    }
    std::size_t units = bytes.size() / 2;
    out.resize(units * 3);
    std::size_t size = 0;
    for (std::size_t i = 0; i < units; i++) {
        std::uint32_t unit = static_cast<std::uint8_t>(bytes[i * 2]) | (static_cast<std::uint8_t>(bytes[i * 2 + 1]) << 8);
        bool high = unit >= 0xD800 && unit <= 0xDBFF;
        if (high && i + 1 < units) {
            std::uint32_t next =
                static_cast<std::uint8_t>(bytes[i * 2 + 2]) | (static_cast<std::uint8_t>(bytes[i * 2 + 3]) << 8);
            if (next >= 0xDC00 && next <= 0xDFFF) {
                unit = 0x10000 + ((unit - 0xD800) << 10) + (next - 0xDC00);
                i++;
            }
        }
        size += appendUtf8(out.data() + size, unit);
    }
    out.resize(size);
    return out;
}

}
