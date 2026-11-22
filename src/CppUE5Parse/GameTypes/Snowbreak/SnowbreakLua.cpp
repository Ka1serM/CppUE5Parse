#include "CppUE5Parse/GameTypes/Snowbreak/SnowbreakLua.h"

#include <string_view>

#include "CppUE5Parse/Crypto/Decryptor.h"

namespace ue5parse {

namespace {

constexpr std::string_view snowbreakLuaKey = "E1671E910EB54736B49908575F08CAFB";

}

void decryptSnowbreakLua(std::span<std::byte> file) {
    AesKey key{};
    for (std::size_t i = 0; i < key.size(); i++)
        key[i] = static_cast<std::byte>(snowbreakLuaKey[i]);
    decryptAesEcb(key, file.first(file.size() / 16 * 16));
}

}
