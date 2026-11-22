#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

struct FinalFantasy7RebirthIndexBuffer {
    std::vector<std::uint32_t> sectionTriangleCounts;
    std::vector<std::uint32_t> indices;
};

//! Final Fantasy VII Rebirth packs its static mesh triangles in batches of 10-bit local indices; no batches means the stock buffer stays.
Result<std::optional<FinalFantasy7RebirthIndexBuffer>> decodeFinalFantasy7RebirthIndexBuffer(std::span<const std::byte> data);

}
