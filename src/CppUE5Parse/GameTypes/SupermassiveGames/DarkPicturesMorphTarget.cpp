#include "CppUE5Parse/GameTypes/SupermassiveGames/DarkPicturesMorphTarget.h"

#include <bit>
#include <cmath>

#include "CppUE5Parse/Core/ByteReader.h"

namespace ue5parse {

namespace {

float halfToFloat(std::uint16_t half) {
    const std::uint32_t sign = static_cast<std::uint32_t>(half & 0x8000u) << 16;
    const std::uint32_t exponent = (half >> 10) & 0x1Fu;
    const std::uint32_t mantissa = half & 0x3FFu;
    if (exponent == 0) {
        const float value = std::ldexp(static_cast<float>(mantissa), -24);
        return sign != 0 ? -value : value;
    }
    if (exponent == 31)
        return std::bit_cast<float>(sign | 0x7F800000u | (mantissa << 13));
    return std::bit_cast<float>(sign | ((exponent + 112) << 23) | (mantissa << 13));
}

std::array<float, 3> readHalfVector(ByteReader& reader, bool bScaled) {
    std::array<float, 3> vector{};
    for (float& value : vector)
        value = halfToFloat(reader.read<std::uint16_t>());
    if (!bScaled)
        return vector;
    const float scale = halfToFloat(reader.read<std::uint16_t>());
    //! A zero scale means the vector is not scaled.
    if (scale == 0.0f)
        return vector;
    for (float& value : vector)
        value *= scale;
    return vector;
}

}

Result<DarkPicturesMorphTargetDeltas> decodeDarkPicturesMorphTargetDeltas(std::span<const std::byte> data, bool bScaledHalfVectors) {
    ByteReader reader(data);
    const std::size_t pairSize = bScaledHalfVectors ? 16 : 12;
    DarkPicturesMorphTargetDeltas result;
    const std::uint32_t batchCount = reader.readCount(12);
    for (std::uint32_t batch = 0; batch < batchCount && !reader.hasFailed(); batch++) {
        const std::span<const std::byte> indices = reader.readBytes(reader.readCount(1));
        const std::uint32_t vertexCount = reader.readCount(pairSize);
        const std::size_t first = result.deltas.size();
        for (std::uint32_t vertex = 0; vertex < vertexCount && !reader.hasFailed(); vertex++) {
            DecodedMorphTargetDelta& delta = result.deltas.emplace_back();
            delta.positionDelta = readHalfVector(reader, bScaledHalfVectors);
            delta.tangentZDelta = readHalfVector(reader, bScaledHalfVectors);
        }
        const std::uint32_t startIndex = reader.read<std::uint32_t>();
        if (reader.hasFailed())
            break;
        if (indices.size() < vertexCount) {
            reader.fail(ErrorCode::InvalidData, "DarkPicturesMorphTargetDeltas.Indices");
            break;
        }
        for (std::uint32_t vertex = 0; vertex < vertexCount; vertex++)
            result.deltas[first + vertex].sourceIndex = startIndex + std::to_integer<std::uint32_t>(indices[vertex]);
    }
    if (Result<void> status = reader.getResult(); !status)
        return std::unexpected(status.error());
    result.bytesRead = reader.getPosition();
    return result;
}

}
