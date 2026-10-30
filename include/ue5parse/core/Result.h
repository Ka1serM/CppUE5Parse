#pragma once

#include <cstdint>
#include <expected>

namespace ue5parse {

enum class ErrorCode : std::uint16_t {
    Io,
    ReadPastEnd,
    BadMagic,
    UnsupportedVersion,
    MissingKey,
    BadAesKey,
    UnknownCompression,
    Decompression,
    MissingChunk,
    MissingFile,
    MissingPackage,
    MissingExport,
    MissingSchema,
    MissingProperty,
    UnknownPropertyType,
    PropertyKindMismatch,
    ClassMismatch,
    NullReference,
    NameCollision,
    InvalidData,
    Cancelled,
};

// Plain data: the error path allocates nothing. where names the check that failed, a string literal.
struct Error {
    ErrorCode code;
    const char* where;
    std::uint64_t offset = 0;
};

template <class T>
using Result = std::expected<T, Error>;

inline std::unexpected<Error> fail(ErrorCode code, const char* where, std::uint64_t offset = 0) {
    return std::unexpected(Error{code, where, offset});
}

const char* getErrorName(ErrorCode code);

}
