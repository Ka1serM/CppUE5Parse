#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Crypto/Decryptor.h"

namespace ue5parse {

enum class ArenaBreakoutInfiniteFile : std::uint8_t { Other, Ini, Lua, Package };

void decryptArenaBreakoutInfinite(std::span<std::byte> data, const AesKey& key, bool isIndex);
ArenaBreakoutInfiniteFile getArenaBreakoutInfiniteFile(std::string_view path);
//! The size of the package summary that the game encrypts, read from the first eight bytes of the file.
std::size_t getArenaBreakoutInfiniteSummarySize(std::span<const std::byte, 8> start);
Result<void> decryptArenaBreakoutInfinitePackage(std::span<std::byte> file);
//! The size of an ini file after decryption, read from its first eight bytes.
std::uint64_t getArenaBreakoutInfiniteIniSize(std::span<const std::byte, 8> start, std::uint64_t storedSize);
Result<std::vector<std::byte>> decryptArenaBreakoutInfiniteIni(std::span<const std::byte> file);
Result<void> decryptArenaBreakoutInfiniteLua(std::span<std::byte> file);

}
