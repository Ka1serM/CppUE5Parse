#include "BlockReader.h"

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/task_group.h>

#include <algorithm>
#include <atomic>
#include <cstring>

namespace ue5parse {

namespace {

// Data bytes of one run: about one L2 cache of output (design §10.2).
constexpr std::uint64_t runBytes = 2u << 20;

std::atomic<std::uint64_t> decodedBlockEpoch{1};

// The block whose data threadBuffer(1) holds. An epoch of 0 holds none.
struct DecodedBlock {
    const FileSource* file = nullptr;
    std::uint64_t position = 0;
    std::uint64_t epoch = 0;

    bool holds(const StoredBlock& block) const {
        return file == block.file && position == block.position && epoch == decodedBlockEpoch.load();
    }
};

thread_local DecodedBlock decodedBlock;

// Points inputs at the compressed bytes of each block. The stored bytes go to the thread's scratch buffer, in one read
// when the blocks follow each other in one file, and are decrypted there.
Result<void> loadStoredBytes(const BlockList& list, std::size_t first, std::span<const StoredBlock> blocks,
                             std::vector<std::span<const std::byte>>& inputs) {
    const StoredBlock& head = blocks.front();
    const StoredBlock& tail = blocks.back();
    std::size_t slack = 0;
    std::uint64_t total = 0;
    bool contiguous = true;
    for (std::size_t i = 0; i < blocks.size(); i++) {
        const StoredBlock& block = blocks[i];
        if (block.decompressor != nullptr) slack = std::max(slack, block.decompressor->getInputSlack());
        total += block.storedSize;
        if (i > 0) contiguous = contiguous && block.file == head.file && block.position >= blocks[i - 1].position + blocks[i - 1].storedSize;
    }
    // Gaps between the blocks are read too; a span much larger than the blocks is read block by block instead.
    std::uint64_t span = tail.position + tail.storedSize - head.position;
    contiguous = contiguous && span <= 2 * total;

    // The decompressor may read slack bytes past a block, so the buffer has them too.
    std::vector<std::byte>& scratch = threadBuffer(0);
    std::size_t needed = static_cast<std::size_t>(contiguous ? span : total) + slack;
    if (scratch.size() < needed) scratch.resize(needed);
    if (contiguous) {
        Result<void> read = head.file->read(head.position, {scratch.data(), static_cast<std::size_t>(span)});
        if (!read) return read;
    }
    std::size_t packed = 0;
    for (std::size_t i = 0; i < blocks.size(); i++) {
        const StoredBlock& block = blocks[i];
        std::size_t at = contiguous ? static_cast<std::size_t>(block.position - head.position) : packed;
        std::span<std::byte> stored{scratch.data() + at, block.storedSize};
        if (!contiguous) {
            Result<void> read = block.file->read(block.position, stored);
            if (!read) return read;
            packed += block.storedSize;
        }
        if (block.encrypted) {
            Result<void> decrypted = list.decrypt(first + i, stored);
            if (!decrypted) return decrypted;
        }
        inputs.push_back(stored.first(block.compressedSize));
    }
    return {};
}

// Decodes the data bytes [start, start + output.size()) on the calling thread.
Result<void> decodeRun(const BlockList& list, std::uint64_t blockSize, std::uint64_t start, std::span<std::byte> output,
                       std::stop_token stop) {
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "readBlocks");
    std::size_t first = static_cast<std::size_t>(start / blockSize);
    std::size_t last = static_cast<std::size_t>((start + output.size() - 1) / blockSize);
    std::vector<StoredBlock> blocks;
    blocks.reserve(last - first + 1);
    for (std::size_t index = first; index <= last; index++) {
        Result<StoredBlock> block = list.getBlock(index);
        if (!block) return std::unexpected(block.error());
        // Checked before a buffer grows, so a corrupt size cannot cause a large allocation.
        if (block->position > block->file->getSize() || block->storedSize > block->file->getSize() - block->position)
            return fail(ErrorCode::InvalidData, "readBlocks.stored", block->position);
        blocks.push_back(*block);
    }
    std::vector<std::span<const std::byte>> inputs;
    inputs.reserve(blocks.size());
    Result<void> loaded = loadStoredBytes(list, first, blocks, inputs);
    if (!loaded) return loaded;

    std::vector<std::byte>& partial = threadBuffer(1);
    std::size_t written = 0;
    std::uint64_t offsetInBlock = start % blockSize;
    for (std::size_t i = 0; i < blocks.size(); i++) {
        const StoredBlock& block = blocks[i];
        if (offsetInBlock >= block.uncompressedSize) return fail(ErrorCode::InvalidData, "readBlocks.range", block.position);
        std::size_t take = static_cast<std::size_t>(std::min<std::uint64_t>(block.uncompressedSize - offsetInBlock, output.size() - written));
        std::span<std::byte> destination = output.subspan(written, take);
        if (block.decompressor == nullptr) {
            if (offsetInBlock + take > block.compressedSize) return fail(ErrorCode::InvalidData, "readBlocks.uncompressed", block.position);
            std::memcpy(destination.data(), inputs[i].data() + offsetInBlock, take);
        } else {
            // Straight into the output when the whole block is wanted and the codec's overrun stays inside this run,
            // where the next block overwrites it.
            std::size_t outputSlack = block.decompressor->getOutputSlack();
            bool direct = offsetInBlock == 0 && take == block.uncompressedSize && output.size() - written - take >= outputSlack;
            if (direct) {
                Result<void> done = block.decompressor->decompress(inputs[i], destination);
                if (!done) return done;
            } else {
                if (!decodedBlock.holds(block)) {
                    decodedBlock = {};
                    if (partial.size() < block.uncompressedSize + outputSlack) partial.resize(block.uncompressedSize + outputSlack);
                    Result<void> done = block.decompressor->decompress(inputs[i], {partial.data(), block.uncompressedSize});
                    if (!done) return done;
                    decodedBlock = {block.file, block.position, decodedBlockEpoch.load()};
                }
                std::memcpy(destination.data(), partial.data() + offsetInBlock, take);
            }
        }
        written += take;
        offsetInBlock = 0;
    }
    return {};
}

}

void forgetDecodedBlocks() { decodedBlockEpoch.fetch_add(1); }

std::vector<std::byte>& threadBuffer(int which) {
    thread_local std::vector<std::byte> buffers[2];
    return buffers[which];
}

Result<void> readBlocks(const BlockList& blocks, std::uint64_t blockSize, std::uint64_t start, std::span<std::byte> output,
                        std::stop_token stop) {
    if (output.empty()) return {};
    // Runs start at block boundaries.
    std::uint64_t runSize = std::max(blockSize, runBytes / blockSize * blockSize);
    std::uint64_t end = start + output.size();
    std::uint64_t firstRun = start / runSize;
    std::size_t runCount = static_cast<std::size_t>((end - 1) / runSize - firstRun + 1);
    if (runCount == 1) return decodeRun(blocks, blockSize, start, output, stop);

    std::vector<Result<void>> results(runCount);
    // Isolated: a thread that waits here runs only these runs, never an outer task that could wait for a load this
    // thread is inside of (design §10.4).
    tbb::this_task_arena::isolate([&] {
        tbb::task_group_context context;
        tbb::parallel_for(std::size_t(0), runCount, [&](std::size_t run) {
            std::uint64_t runStart = std::max(start, (firstRun + run) * runSize);
            std::uint64_t runEnd = std::min(end, (firstRun + run + 1) * runSize);
            results[run] = decodeRun(blocks, blockSize, runStart, output.subspan(runStart - start, runEnd - runStart), stop);
            if (!results[run]) context.cancel_group_execution();
        }, context);
    });
    // A run that the first error cancelled keeps its empty result, so this finds the error.
    for (Result<void>& result : results)
        if (!result) return result;
    return {};
}

}
