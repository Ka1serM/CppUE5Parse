#include "CppUE5Parse/GameTypes/Rennsport/RennsportAes.h"

#include <array>
#include <cstdint>
#include <cstring>

#include <mbedtls/aes.h>

namespace ue5parse {

namespace {

constexpr std::array<std::uint8_t, 16> entryIv = {0x7C, 0x8D, 0x5C, 0x6C, 0x70, 0x52, 0xA2, 0x41, 0x0C, 0xA5, 0xC3, 0x70, 0xB9, 0x9D, 0x23, 0x3B};
constexpr std::array<std::uint8_t, 12> indexIv = {0x75, 0x7C, 0xE4, 0x79, 0x62, 0xF4, 0xC7, 0x28, 0xF9, 0x89, 0x83, 0x6F};
constexpr std::array<std::uint8_t, 12> directoryIndexIv = {0xD1, 0xDC, 0x32, 0x32, 0x8E, 0xB3, 0x7F, 0xE2, 0x76, 0x49, 0xC9, 0xDC};

//! The game decrypts this block with the user key to get the AES key of the pak files.
constexpr std::array<std::uint8_t, 32> wrappedKey = {0x8E, 0x53, 0x68, 0xB7, 0x6B, 0x5A, 0xA3, 0xBA, 0xD8, 0xF9, 0xE0, 0x18, 0x75, 0x4C, 0x9A, 0x12,
                                                     0x66, 0x23, 0x0D, 0xAC, 0x19, 0xA2, 0x77, 0xE4, 0xA3, 0x2C, 0xD1, 0xD9, 0x2E, 0x92, 0x45, 0x50};

void decryptCbc(std::span<std::byte> data, const AesKey& key, std::span<const std::byte> chain) {
    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_dec(&context, reinterpret_cast<const unsigned char*>(key.data()), 256);
    std::array<std::byte, 16> previous{};
    if (chain.size() == previous.size())
        std::memcpy(previous.data(), chain.data(), previous.size());
    else
        std::memcpy(previous.data(), entryIv.data(), previous.size());
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        std::array<std::byte, 16> stored{};
        std::memcpy(stored.data(), data.data() + offset, 16);
        auto* block = reinterpret_cast<unsigned char*>(data.data() + offset);
        mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_DECRYPT, block, block);
        for (std::size_t i = 0; i < 16; i++)
            data[offset + i] ^= previous[i];
        previous = stored;
    }
    mbedtls_aes_free(&context);
}

//! CUE4Parse runs AES-GCM without its final step, so the keystream starts at counter 2 and the last 16 bytes, which it takes as the tag, stay zero.
void decryptGcmStream(std::span<std::byte> data, const AesKey& key, const std::array<std::uint8_t, 12>& iv) {
    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_enc(&context, reinterpret_cast<const unsigned char*>(key.data()), 256);
    std::size_t size = data.size() >= 16 ? data.size() - 16 : 0;
    std::array<unsigned char, 16> counter{};
    std::array<unsigned char, 16> stream{};
    std::memcpy(counter.data(), iv.data(), iv.size());
    std::uint32_t block = 2;
    for (std::size_t offset = 0; offset < size; offset += 16, block++) {
        counter[12] = static_cast<unsigned char>(block >> 24);
        counter[13] = static_cast<unsigned char>(block >> 16);
        counter[14] = static_cast<unsigned char>(block >> 8);
        counter[15] = static_cast<unsigned char>(block);
        mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, counter.data(), stream.data());
        for (std::size_t i = 0; i < 16 && offset + i < size; i++)
            data[offset + i] ^= static_cast<std::byte>(stream[i]);
    }
    mbedtls_aes_free(&context);
    std::memset(data.data() + size, 0, data.size() - size);
}

}

void decryptRennsport(std::span<std::byte> data, const AesKey& key, EncryptedPart part, std::span<const std::byte> chain) {
    AesKey derived{};
    std::memcpy(derived.data(), wrappedKey.data(), derived.size());
    decryptAesEcb(key, derived);
    if (part == EncryptedPart::PakIndex)
        decryptGcmStream(data, derived, indexIv);
    else if (part == EncryptedPart::PakDirectoryIndex)
        decryptGcmStream(data, derived, directoryIndexIv);
    else
        decryptCbc(data, derived, chain);
}

}
