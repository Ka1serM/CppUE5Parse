#pragma once

// Internal to the library: decoding a range of compression blocks, shared by the IoStore and pak readers.

#include <cstddef>
#include <cstdint>
#include <span>
#include <stop_token>
#include <vector>

#include "ue5parse/compress/Decompressor.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/io/FileSource.h"

namespace ue5parse {

constexpr std::size_t aesBlock = 16;

inline std::uint64_t alignUp(std::uint64_t value, std::uint64_t alignment) { return (value + alignment - 1) / alignment * alignment; }

// A reusable buffer for each thread (design §3, the task scratch): stored bytes and partial blocks. A user must not
// wait for other tasks while it holds one, because the waiting thread can run another read that uses it.
std::vector<std::byte>& threadBuffer(int which);

// Where one compression block is stored, and how to decode it.
struct StoredBlock {
    const FileSource* file = nullptr;
    std::uint64_t position = 0;     // of the stored bytes in the file
    std::size_t compressedSize = 0;
    std::size_t storedSize = 0;     // compressedSize, rounded up to the AES block when encrypted
    std::size_t uncompressedSize = 0;
    const Decompressor* decompressor = nullptr;   // null: stored without compression
    bool encrypted = false;
};

// The blocks of one container (an IoStore container, or one pak entry).
class BlockList {
public:
    virtual ~BlockList() = default;
    virtual Result<StoredBlock> getBlock(std::size_t index) const = 0;
    // Decrypts the stored bytes of an encrypted block in place.
    virtual Result<void> decrypt(std::size_t index, std::span<std::byte> stored) const = 0;
};

// Decodes the data bytes [start, start + output.size()) of blocks that each hold blockSize bytes of data (the last
// one may hold fewer). Reads only these blocks. A large range decodes as parallel runs of about 2 MB (design §10.3):
// each run reads its stored bytes in one request, or uses them in place in a mapped file, then decrypts and
// decompresses them while they are in cache. Each thread keeps the last block that it decoded only in part, so reads
// of neighbouring small ranges (the exports of a package) decode a shared block once.
Result<void> readBlocks(const BlockList& blocks, std::uint64_t blockSize, std::uint64_t start, std::span<std::byte> output,
                        std::stop_token stop);

// Drops the decoded block of every thread. A container calls it when it closes its files, because a file opened later
// can have the address of a closed one.
void forgetDecodedBlocks();

}
