#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ue5parse {

std::array<std::uint8_t, 32> hashBlake3(std::span<const std::byte> input);

}
