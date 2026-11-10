#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

class ByteReader {
public:
    explicit ByteReader(std::span<const std::byte> data) : data(data) {}

    template <class T>
        requires std::is_trivially_copyable_v<T>
    T read() {
        T value{};
        if (!require(sizeof(T), "ByteReader.read"))
            return value;
        std::memcpy(&value, data.data() + position, sizeof(T));
        position += sizeof(T);
        return value;
    }

    template <class T>
        requires std::is_trivially_copyable_v<T>
    void readInto(std::span<T> output) {
        //! An empty span may have a null pointer, which memcpy must not get.
        if (output.empty())
            return;
        if (!require(output.size_bytes(), "ByteReader.readInto"))
            return;
        std::memcpy(output.data(), data.data() + position, output.size_bytes());
        position += output.size_bytes();
    }

    std::span<const std::byte> readBytes(std::size_t size) {
        if (!require(size, "ByteReader.readBytes"))
            return {};
        std::span<const std::byte> bytes = data.subspan(position, size);
        position += size;
        return bytes;
    }

    std::uint32_t readCount(std::size_t elementSize) {
        std::int32_t count = read<std::int32_t>();
        if (count < 0) {
            fail(ErrorCode::InvalidData, "ByteReader.readCount");
            return 0;
        }
        if (elementSize != 0 && static_cast<std::size_t>(count) > getRemaining() / elementSize) {
            fail(ErrorCode::ReadPastEnd, "ByteReader.readCount");
            return 0;
        }
        return static_cast<std::uint32_t>(count);
    }

    std::span<const std::byte> readStringBytes(bool& wide);
    std::string_view readAnsiString();

    void skip(std::size_t size) {
        if (!require(size, "ByteReader.skip"))
            return;
        position += size;
    }

    void seek(std::size_t newPosition) {
        if (newPosition > data.size()) {
            fail(ErrorCode::ReadPastEnd, "ByteReader.seek");
            return;
        }
        position = newPosition;
    }

    void align(std::size_t alignment) {
        std::size_t aligned = (position + alignment - 1) / alignment * alignment;
        seek(aligned);
    }

    void fail(ErrorCode code, const char* where) {
        if (failed)
            return;
        failed = true;
        error = Error{code, where, position};
    }

    bool hasFailed() const { return failed; }
    const Error& getError() const { return error; }
    Result<void> getResult() const {
        if (failed)
            return std::unexpected(error);
        return {};
    }

    std::size_t getPosition() const { return position; }
    std::size_t getSize() const { return data.size(); }
    std::size_t getRemaining() const { return data.size() - position; }
    std::span<const std::byte> getData() const { return data; }

private:
    bool require(std::size_t size, const char* where) {
        if (failed)
            return false;
        if (size <= data.size() - position)
            return true;
        fail(ErrorCode::ReadPastEnd, where);
        return false;
    }

    std::span<const std::byte> data;
    std::size_t position = 0;
    bool failed = false;
    Error error{};
};

}
