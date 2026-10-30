#include <cstdio>
#include <cstdlib>
#include <string>

#include <city.h>

#include "ue5parse/core/ByteReader.h"
#include "ue5parse/core/Check.h"
#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"

namespace ue5parse {

const char* getErrorName(ErrorCode code) {
    switch (code) {
    case ErrorCode::Io: return "Io";
    case ErrorCode::ReadPastEnd: return "ReadPastEnd";
    case ErrorCode::BadMagic: return "BadMagic";
    case ErrorCode::UnsupportedVersion: return "UnsupportedVersion";
    case ErrorCode::MissingKey: return "MissingKey";
    case ErrorCode::BadAesKey: return "BadAesKey";
    case ErrorCode::UnknownCompression: return "UnknownCompression";
    case ErrorCode::Decompression: return "Decompression";
    case ErrorCode::MissingChunk: return "MissingChunk";
    case ErrorCode::MissingFile: return "MissingFile";
    case ErrorCode::MissingPackage: return "MissingPackage";
    case ErrorCode::MissingExport: return "MissingExport";
    case ErrorCode::MissingSchema: return "MissingSchema";
    case ErrorCode::MissingProperty: return "MissingProperty";
    case ErrorCode::UnknownPropertyType: return "UnknownPropertyType";
    case ErrorCode::PropertyKindMismatch: return "PropertyKindMismatch";
    case ErrorCode::ClassMismatch: return "ClassMismatch";
    case ErrorCode::NullReference: return "NullReference";
    case ErrorCode::NameCollision: return "NameCollision";
    case ErrorCode::InvalidData: return "InvalidData";
    case ErrorCode::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

void checkFailed(const char* condition, const char* file, int line) {
    std::fprintf(stderr, "ue5parse: check failed: %s at %s:%d\n", condition, file, line);
    std::abort();
}

std::span<const std::byte> ByteReader::readStringBytes(bool& wide) {
    wide = false;
    std::int32_t length = read<std::int32_t>();
    if (hasFailed() || length == 0) return {};
    if (length > 0) {
        std::span<const std::byte> bytes = readBytes(static_cast<std::size_t>(length));
        if (hasFailed()) return {};
        return bytes.first(bytes.size() - 1);
    }
    if (length == INT32_MIN) {
        fail(ErrorCode::InvalidData, "ByteReader.readStringBytes");
        return {};
    }
    wide = true;
    std::size_t units = static_cast<std::size_t>(-length);
    if (units > getRemaining() / 2) {
        fail(ErrorCode::ReadPastEnd, "ByteReader.readStringBytes");
        return {};
    }
    std::span<const std::byte> bytes = readBytes(units * 2);
    return bytes.first(bytes.size() - 2);
}

std::string_view ByteReader::readAnsiString() {
    bool wide = false;
    std::span<const std::byte> bytes = readStringBytes(wide);
    if (wide) {
        fail(ErrorCode::InvalidData, "ByteReader.readAnsiString");
        return {};
    }
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

namespace {

char toLowerAscii(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; }

}

std::uint64_t hashNameRuntime(std::string_view text) {
    char lower[1024];
    std::size_t size = text.size() < sizeof(lower) ? text.size() : sizeof(lower);
    for (std::size_t i = 0; i < size; i++) lower[i] = toLowerAscii(text[i]);
    return CityHash64(lower, size);
}

static_assert(hashName("None") == FName::noneHash);

std::string FName::toString() const {
    std::string out;
    appendTo(out);
    return out;
}

void FName::appendTo(std::string& out) const {
    out.append(text);
    if (number == 0) return;
    out.push_back('_');
    out.append(std::to_string(number - 1));
}

}
