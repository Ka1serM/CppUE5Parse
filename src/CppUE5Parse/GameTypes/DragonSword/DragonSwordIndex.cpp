#include "CppUE5Parse/GameTypes/DragonSword/DragonSwordIndex.h"

#include <cstdint>
#include <cstring>

namespace ue5parse {

namespace {

class IndexCursor {
public:
    explicit IndexCursor(std::span<std::byte> data) : data(data) {}

    template <class T>
    bool read(T& value) {
        if (sizeof(T) > data.size() - position)
            return false;
        std::memcpy(&value, data.data() + position, sizeof(T));
        position += sizeof(T);
        return true;
    }

    bool skip(std::size_t size) {
        if (size > data.size() - position)
            return false;
        position += size;
        return true;
    }

    std::span<std::byte> take(std::size_t size) {
        if (size > data.size() - position)
            return {};
        std::span<std::byte> taken = data.subspan(position, size);
        position += size;
        return taken;
    }

    //! The key is the last character, so the decoded string ends with a null character.
    bool decodeString() {
        std::int32_t length = 0;
        if (!read(length) || length == INT32_MIN)
            return false;
        if (length == 0)
            return true;
        if (length > 0) {
            std::span<std::byte> bytes = take(static_cast<std::size_t>(length));
            if (bytes.empty())
                return false;
            std::byte key = bytes.back();
            for (std::byte& value : bytes)
                value ^= key;
            return true;
        }
        std::size_t units = static_cast<std::size_t>(-static_cast<std::int64_t>(length));
        if (units > (data.size() - position) / 2)
            return false;
        std::span<std::byte> bytes = take(units * 2);
        std::byte low = bytes[bytes.size() - 2];
        std::byte high = bytes[bytes.size() - 1];
        for (std::size_t i = 0; i < bytes.size(); i += 2) {
            bytes[i] ^= low;
            bytes[i + 1] ^= high;
        }
        return true;
    }

private:
    std::span<std::byte> data;
    std::size_t position = 0;
};

}

Result<void> decodeDragonSwordPrimaryIndex(std::span<std::byte> index) {
    IndexCursor cursor{index};
    if (!cursor.decodeString())
        return fail(ErrorCode::InvalidData, "decodeDragonSwordPrimaryIndex.mountPoint");
    std::uint32_t hasPathHashIndex = 0;
    if (!cursor.skip(4 + 8) || !cursor.read(hasPathHashIndex))
        return fail(ErrorCode::InvalidData, "decodeDragonSwordPrimaryIndex.pathHashIndex");
    if (hasPathHashIndex == 0)
        return fail(ErrorCode::UnsupportedVersion, "decodeDragonSwordPrimaryIndex.pathHashIndex");
    std::uint32_t hasDirectoryIndex = 0;
    if (!cursor.skip(8 + 8 + 20) || !cursor.read(hasDirectoryIndex))
        return fail(ErrorCode::InvalidData, "decodeDragonSwordPrimaryIndex.directoryIndex");
    if (hasDirectoryIndex == 0)
        return fail(ErrorCode::UnsupportedVersion, "decodeDragonSwordPrimaryIndex.directoryIndex");
    std::int32_t encodedSize = 0;
    if (!cursor.skip(8 + 8 + 20) || !cursor.read(encodedSize) || encodedSize < 0)
        return fail(ErrorCode::InvalidData, "decodeDragonSwordPrimaryIndex.encodedSize");
    if (encodedSize == 0)
        return {};
    std::span<std::byte> encoded = cursor.take(static_cast<std::size_t>(encodedSize));
    if (encoded.size() <= 5)
        return fail(ErrorCode::InvalidData, "decodeDragonSwordPrimaryIndex.encodedEntries");
    std::byte key = encoded[5];
    for (std::byte& value : encoded)
        value ^= key;
    return {};
}

Result<void> decodeDragonSwordDirectoryIndex(std::span<std::byte> index) {
    IndexCursor cursor{index};
    std::int32_t directoryCount = 0;
    if (!cursor.read(directoryCount) || directoryCount < 0)
        return fail(ErrorCode::InvalidData, "decodeDragonSwordDirectoryIndex.count");
    for (std::int32_t i = 0; i < directoryCount; i++) {
        std::int32_t fileCount = 0;
        if (!cursor.decodeString() || !cursor.read(fileCount) || fileCount < 0)
            return fail(ErrorCode::InvalidData, "decodeDragonSwordDirectoryIndex.directory");
        for (std::int32_t j = 0; j < fileCount; j++)
            if (!cursor.decodeString() || !cursor.skip(4))
                return fail(ErrorCode::InvalidData, "decodeDragonSwordDirectoryIndex.file");
    }
    return {};
}

}
