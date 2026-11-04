#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stop_token>
#include <vector>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/IO/FileSource.h"

namespace ue5parse {

constexpr std::size_t aesBlock = 16;

inline std::uint64_t alignUp(std::uint64_t value, std::uint64_t alignment) { return (value + alignment - 1) / alignment * alignment; }

//! Do not wait for other tasks while holding it: the waiting thread can run another read that uses it.
std::vector<std::byte>& threadBuffer(int which);

struct StoredBlock {
    const FileSource* file = nullptr;
    std::uint64_t position = 0;
    std::size_t compressedSize = 0;
    std::size_t storedSize = 0;
    std::size_t uncompressedSize = 0;
    const Decompressor* decompressor = nullptr;
    bool encrypted = false;
};

class BlockList {
public:
    virtual ~BlockList() = default;
    virtual Result<StoredBlock> getBlock(std::size_t index) const = 0;
    virtual Result<void> decrypt(std::size_t index, std::span<std::byte> stored) const = 0;
};

Result<void> readBlocks(const BlockList& blocks, std::uint64_t blockSize, std::uint64_t start, std::span<std::byte> output,
                        std::stop_token stop);

//! A file opened later can have the address of a closed one.
void forgetDecodedBlocks();

}
