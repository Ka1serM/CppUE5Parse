#include "CppUE5Parse/GameTypes/AceCombat7/AceCombat7Decrypt.h"

namespace ue5parse {

namespace {

constexpr std::uint32_t keyRows = 217;
constexpr std::uint32_t keyColumns = 1024;

std::uint32_t getNameKey(std::string_view fileName) {
    std::uint32_t key = 0;
    for (char character : fileName) {
        const std::uint32_t upper = character >= 'a' && character <= 'z' ? static_cast<std::uint32_t>(character - 'a' + 'A') : static_cast<std::uint8_t>(character);
        key ^= upper;
        const std::uint32_t mixed = ~((key * 8) ^ key);
        key = ((mixed >> 7) & 1) | (key + key);
    }
    return key;
}

}

AceCombat7Stream getAceCombat7Stream(std::string_view fileName) {
    constexpr std::uint32_t dataOffset = 4;
    const std::uint32_t nameKey = getNameKey(fileName);
    const std::uint64_t rowSeed = static_cast<std::uint32_t>(static_cast<std::uint64_t>(nameKey) * 7) + std::uint64_t{dataOffset};
    const std::uint64_t columnSeed = static_cast<std::uint32_t>(static_cast<std::uint64_t>(nameKey) * 11) + std::uint64_t{dataOffset};
    return {static_cast<std::uint32_t>(rowSeed % keyRows), static_cast<std::uint32_t>(columnSeed % keyColumns)};
}

void decryptAceCombat7(std::span<std::byte> data, AceCombat7Stream start, std::uint64_t position, std::span<const std::byte, aceCombat7KeySize> key) {
    std::uint32_t row = static_cast<std::uint32_t>((start.row + position) % keyRows);
    std::uint32_t column = static_cast<std::uint32_t>((start.column + position) % keyColumns);
    for (std::byte& value : data) {
        value ^= key[row * keyColumns + column] ^ std::byte{0x77};
        row = row + 1 == keyRows ? 0 : row + 1;
        column = column + 1 == keyColumns ? 0 : column + 1;
    }
}

}
