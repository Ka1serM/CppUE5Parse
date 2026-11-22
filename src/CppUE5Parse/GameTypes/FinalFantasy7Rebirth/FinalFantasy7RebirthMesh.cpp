#include "CppUE5Parse/GameTypes/FinalFantasy7Rebirth/FinalFantasy7RebirthMesh.h"

#include "CppUE5Parse/Core/ByteReader.h"

namespace ue5parse {

namespace {

struct Batch {
    std::uint32_t vertexCount;
    std::uint32_t firstVertex;
    std::uint32_t triangleCount;
    std::uint32_t firstTriangle;
};

struct SectionRange {
    std::uint32_t firstBatch;
    std::uint32_t batchCount;
};

constexpr std::size_t headerSize = 28;
constexpr std::size_t batchInfoSize = 80;
constexpr std::size_t lodSize = 64;
constexpr std::size_t boundsSetSize = 416;
constexpr std::size_t lodInfoSize = 96;
constexpr std::size_t lodInfoBatchesOffset = 24;

template <class T>
std::vector<T> readArray(ByteReader& reader) {
    std::vector<T> values(reader.readCount(sizeof(T)));
    reader.readInto(std::span<T>(values));
    return values;
}

void skipArray(ByteReader& reader, std::size_t elementSize) {
    reader.skip(std::size_t(reader.readCount(elementSize)) * elementSize);
}

std::vector<SectionRange> readSectionRanges(ByteReader& reader) {
    std::uint32_t count = reader.readCount(lodInfoSize);
    std::vector<SectionRange> ranges(count);
    for (SectionRange& range : ranges) {
        std::span<const std::byte> info = reader.readBytes(lodInfoSize);
        if (reader.hasFailed())
            break;
        std::memcpy(&range, info.data() + lodInfoBatchesOffset, sizeof(range));
    }
    return ranges;
}

}

Result<std::optional<FinalFantasy7RebirthIndexBuffer>> decodeFinalFantasy7RebirthIndexBuffer(std::span<const std::byte> data) {
    ByteReader reader(data);
    reader.skip(headerSize);
    std::vector<Batch> batches = readArray<Batch>(reader);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (batches.empty())
        return std::nullopt;
    std::vector<std::uint32_t> vertices = readArray<std::uint32_t>(reader);
    std::vector<std::uint32_t> triangles = readArray<std::uint32_t>(reader);
    skipArray(reader, batchInfoSize);
    skipArray(reader, lodSize);
    skipArray(reader, sizeof(std::int32_t));
    skipArray(reader, boundsSetSize);
    reader.skip(sizeof(std::int32_t));
    std::vector<std::uint32_t> sectionLods = readArray<std::uint32_t>(reader);
    std::vector<SectionRange> lods = readSectionRanges(reader);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());

    std::vector<std::uint32_t> all;
    all.reserve(triangles.size() * 3);
    for (const Batch& batch : batches) {
        if (batch.firstTriangle > triangles.size() || batch.triangleCount > triangles.size() - batch.firstTriangle)
            return fail(ErrorCode::InvalidData, "decodeFinalFantasy7RebirthIndexBuffer.batch");
        for (std::uint32_t i = 0; i < batch.triangleCount; i++) {
            std::uint32_t packed = triangles[batch.firstTriangle + i];
            for (int corner = 0; corner < 3; corner++) {
                std::size_t vertex = std::size_t(batch.firstVertex) + ((packed >> (corner * 10)) & 0x3FF);
                if (vertex >= vertices.size())
                    return fail(ErrorCode::InvalidData, "decodeFinalFantasy7RebirthIndexBuffer.vertex");
                all.push_back(vertices[vertex]);
            }
        }
    }

    FinalFantasy7RebirthIndexBuffer buffer;
    buffer.sectionTriangleCounts.reserve(sectionLods.size());
    for (std::uint32_t lodIndex : sectionLods) {
        if (lodIndex >= lods.size())
            return fail(ErrorCode::InvalidData, "decodeFinalFantasy7RebirthIndexBuffer.section");
        const SectionRange& range = lods[lodIndex];
        std::size_t begin = std::size_t(range.firstBatch) * 3;
        std::size_t end = begin + std::size_t(range.batchCount) * 3;
        if (end > all.size())
            return fail(ErrorCode::InvalidData, "decodeFinalFantasy7RebirthIndexBuffer.range");
        buffer.sectionTriangleCounts.push_back(range.batchCount);
        buffer.indices.insert(buffer.indices.end(), all.begin() + static_cast<std::ptrdiff_t>(begin), all.begin() + static_cast<std::ptrdiff_t>(end));
    }
    return buffer;
}

}
