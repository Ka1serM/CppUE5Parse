#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>

#include <mbedtls/aes.h>
#include <mbedtls/md5.h>

#include "CppUE5Parse/Crypto/Decryptor.h"
#include "Test.h"

namespace {

using ue5parse::AesDecryptor;
using ue5parse::AesKey;
using ue5parse::DecryptRequest;
using ue5parse::EGame;
using ue5parse::EncryptedPart;
using ue5parse::FGuid;
using ue5parse::KeyRing;

using Block = std::array<std::byte, 32>;

AesKey makeKey() {
    AesKey key{};
    for (std::size_t i = 0; i < key.size(); i++)
        key[i] = static_cast<std::byte>(i * 7 + 3);
    return key;
}

Block makePlain() {
    Block plain{};
    for (std::size_t i = 0; i < plain.size(); i++)
        plain[i] = static_cast<std::byte>(0xA0 + i);
    return plain;
}

void encrypt(const AesKey& key, unsigned int bits, std::span<std::byte> data) {
    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_enc(&context, reinterpret_cast<const unsigned char*>(key.data()), bits);
    auto* bytes = reinterpret_cast<unsigned char*>(data.data());
    for (std::size_t offset = 0; offset < data.size(); offset += 16)
        mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, bytes + offset, bytes + offset);
    mbedtls_aes_free(&context);
}

bool decryptAs(EGame game, const AesKey& key, std::span<std::byte> data, EncryptedPart part, std::string_view name = {}) {
    KeyRing ring;
    ring.add(FGuid{}, key);
    AesDecryptor decryptor(ring);
    DecryptRequest request;
    request.data = data;
    request.part = part;
    request.game = game;
    request.archiveName = name;
    return decryptor.decrypt(request).has_value();
}

std::uint8_t reverseBits(std::uint8_t value) {
    std::uint8_t result = 0;
    for (int i = 0; i < 8; i++)
        result = static_cast<std::uint8_t>(result | (((value >> i) & 1) << (7 - i)));
    return result;
}

}

TEST(dreamStarDecryptRotatesAfterAes) {
    AesKey key = makeKey();
    Block plain = makePlain();
    Block stored = plain;
    for (std::byte& value : stored) {
        auto byte = static_cast<std::uint8_t>(value);
        value = static_cast<std::byte>(static_cast<std::uint8_t>(((byte >> 5) | (byte << 3)) ^ 0x5F));
    }
    encrypt(key, 256, stored);
    REQUIRE(decryptAs(EGame::GAME_DreamStar, key, stored, EncryptedPart::PakEntry));
    CHECK(stored == plain);
}

TEST(infinityNikkiDecryptMasksBlockEnds) {
    AesKey key = makeKey();
    Block plain = makePlain();
    Block stored = plain;
    for (std::size_t offset = 0; offset < stored.size(); offset += 16) {
        stored[offset] ^= key.back();
        stored[offset + 15] ^= key.front();
    }
    encrypt(key, 256, stored);
    REQUIRE(decryptAs(EGame::GAME_InfinityNikki, key, stored, EncryptedPart::PakIndex));
    CHECK(stored == plain);
}

TEST(bloodBowl3DecryptUsesAes128) {
    AesKey key = makeKey();
    Block plain = makePlain();
    Block stored = plain;
    encrypt(key, 128, stored);
    REQUIRE(decryptAs(EGame::GAME_BloodBowl3, key, stored, EncryptedPart::PakEntry));
    CHECK(stored == plain);
}

TEST(snowbreakDecryptDerivesArchiveKey) {
    AesKey key = makeKey();
    std::string name = "pakchunk0-windows";
    unsigned char digest[16];
    mbedtls_md5(reinterpret_cast<const unsigned char*>(name.data()), name.size(), digest);
    constexpr char hex[] = "0123456789abcdef";
    AesKey derived{};
    for (std::size_t i = 0; i < 16; i++) {
        derived[i * 2] = static_cast<std::byte>(hex[digest[i] >> 4]);
        derived[i * 2 + 1] = static_cast<std::byte>(hex[digest[i] & 0xF]);
    }
    encrypt(key, 256, derived);
    Block plain = makePlain();
    Block stored = plain;
    encrypt(derived, 256, stored);
    REQUIRE(decryptAs(EGame::GAME_Snowbreak, key, stored, EncryptedPart::PakEntry, "Paks/PakChunk0-Windows.pak"));
    CHECK(stored == plain);
}

TEST(snowbreakZeroKeyLeavesDataUnchanged) {
    Block plain = makePlain();
    Block stored = plain;
    REQUIRE(decryptAs(EGame::GAME_Snowbreak, AesKey{}, stored, EncryptedPart::PakIndex, "pakchunk0.pak"));
    CHECK(stored == plain);
}

TEST(rocoKingdomWorldDecryptReversesBitsAndKey) {
    AesKey key = makeKey();
    AesKey mutated{};
    for (std::size_t i = 0; i + 1 < key.size(); i++)
        mutated[i] = key[key.size() - i - 2];
    mutated.back() = key.back();
    Block plain = makePlain();
    Block stored = plain;
    encrypt(mutated, 256, stored);
    for (std::byte& value : stored)
        value = static_cast<std::byte>(reverseBits(static_cast<std::uint8_t>(value)));
    REQUIRE(decryptAs(EGame::GAME_RocoKingdomWorld, key, stored, EncryptedPart::PakEntry));
    CHECK(stored == plain);
}

TEST(dragonSwordDecryptMasksIndex) {
    AesKey key = makeKey();
    Block plain = makePlain();
    Block stored = plain;
    encrypt(key, 256, stored);
    REQUIRE(decryptAs(EGame::GAME_DragonSwordAwakening, key, stored, EncryptedPart::PakIndex));
    for (std::size_t i = 0; i < plain.size(); i++)
        CHECK(stored[i] == (plain[i] ^ plain[2]));
}

TEST(needForSpeedMobileBlocksAreNotEncrypted) {
    Block plain = makePlain();
    Block stored = plain;
    REQUIRE(decryptAs(EGame::GAME_NeedForSpeedMobile, makeKey(), stored, EncryptedPart::CompressionBlock));
    CHECK(stored == plain);
}

TEST(onePieceAmbitionDecryptNeedsNoKey) {
    Block plain = makePlain();
    Block stored{};
    for (std::size_t offset = 0; offset < plain.size(); offset += 16) {
        std::uint64_t first;
        std::uint64_t second;
        std::memcpy(&first, plain.data() + offset, 8);
        std::memcpy(&second, plain.data() + offset + 8, 8);
        std::uint64_t high = first ^ 0xD6D6D6D6D6D6D6D6;
        std::uint64_t low = second ^ first;
        std::memcpy(stored.data() + offset, &low, 8);
        std::memcpy(stored.data() + offset + 8, &high, 8);
    }
    AesDecryptor decryptor(KeyRing{});
    DecryptRequest request;
    request.data = stored;
    request.part = EncryptedPart::PakEntry;
    request.game = EGame::GAME_OnePieceAmbition;
    REQUIRE(decryptor.decrypt(request).has_value());
    CHECK(stored == plain);
}
