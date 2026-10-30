#include <city.h>

#include "ue5parse/core/Text.h"
#include "ue5parse/package/ZenTypes.h"

namespace ue5parse {

namespace {

// UE's NAME_SIZE: no name or package path is longer.
constexpr std::size_t maxNameLength = 1024;

}

FPackageId FPackageId::fromName(std::string_view packageName) {
    // Lowercase UTF-16, as TCHAR on the Windows cooker. Package paths are ASCII in practice; other bytes are widened
    // as Latin-1, which only matters for the hash of a name that would not resolve anyway.
    char16_t wide[maxNameLength];
    std::size_t length = packageName.size() < maxNameLength ? packageName.size() : maxNameLength;
    for (std::size_t i = 0; i < length; i++) {
        unsigned char c = static_cast<unsigned char>(packageName[i]);
        wide[i] = static_cast<char16_t>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    }
    return FPackageId{CityHash64(reinterpret_cast<const char*>(wide), length * sizeof(char16_t))};
}

Result<std::vector<FName>> readNameBatch(ByteReader& reader, Arena& storage) {
    std::vector<FName> names;
    std::uint32_t count = reader.read<std::uint32_t>();
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    if (count == 0) return names;
    std::uint32_t stringBytes = reader.read<std::uint32_t>();
    reader.skip(8);   // hash version
    // Each name has an 8-byte hash and a 2-byte header.
    if (!reader.hasFailed() && count > reader.getRemaining() / 10) return fail(ErrorCode::ReadPastEnd, "readNameBatch.count");
    reader.skip(std::size_t(count) * 8);   // the saved hashes; names are hashed the library's way below
    std::span<const std::byte> headers = reader.readBytes(std::size_t(count) * 2);
    std::span<const std::byte> strings = reader.readBytes(stringBytes);
    if (reader.hasFailed()) return std::unexpected(reader.getError());

    names.reserve(count);
    std::size_t position = 0;
    for (std::uint32_t i = 0; i < count; i++) {
        std::uint8_t high = static_cast<std::uint8_t>(headers[i * 2]);
        std::uint8_t low = static_cast<std::uint8_t>(headers[i * 2 + 1]);
        bool wide = (high & 0x80) != 0;
        std::size_t length = (std::size_t(high & 0x7F) << 8) | low;
        std::size_t bytes = wide ? length * 2 : length;
        if (bytes > strings.size() - position) return fail(ErrorCode::InvalidData, "readNameBatch.strings");
        std::string_view text = StringView(strings.subspan(position, bytes), wide).toUtf8(storage);
        position += bytes;
        names.push_back(FName(text, hashName(text), 0));
    }
    return names;
}

FName makeName(std::span<const FName> names, std::uint32_t index, std::uint32_t number) {
    if (index >= names.size()) return FName();
    const FName& base = names[index];
    return FName(base.getText(), base.getHash(), number);
}

}
