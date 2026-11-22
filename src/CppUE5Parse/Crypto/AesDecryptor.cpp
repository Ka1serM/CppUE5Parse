#include <algorithm>
#include <cstring>

#include <mbedtls/aes.h>
#include <mbedtls/md5.h>

#include "CppUE5Parse/Crypto/Decryptor.h"
#include "CppUE5Parse/GameTypes/ApexLegendsMobile/ApexLegendsMobileAes.h"
#include "CppUE5Parse/GameTypes/MarvelRivals/MarvelRivalsAes.h"
#include "CppUE5Parse/GameTypes/Undawn/UndawnAes.h"
#include "CppUE5Parse/GameTypes/PaxDei/PaxDeiAes.h"
#include "CppUE5Parse/GameTypes/Rennsport/RennsportAes.h"
#include "CppUE5Parse/GameTypes/FreeStyleRebound/FreeStyleReboundAes.h"
#include "CppUE5Parse/GameTypes/FunkoFusion/FunkoFusionAes.h"
#include "CppUE5Parse/GameTypes/TonyHawkProSkater12/TonyHawkProSkater12Aes.h"
#include "CppUE5Parse/GameTypes/Spectre/SpectreAes.h"
#include "CppUE5Parse/GameTypes/Splitgate2/Splitgate2Aes.h"
#include "CppUE5Parse/GameTypes/MindsEye/MindsEyeAes.h"
#include "CppUE5Parse/GameTypes/UnchartedWatersOrigin/UnchartedWatersOriginAes.h"
#include "CppUE5Parse/GameTypes/AssaultFireFuture/AssaultFireFutureAes.h"
#include "CppUE5Parse/GameTypes/ArcRaiders/ArcRaidersAes.h"
#include "CppUE5Parse/GameTypes/ArenaBreakoutInfinite/ArenaBreakoutInfiniteAes.h"
#include "CppUE5Parse/GameTypes/ProSpi/ProSpiDecrypt.h"

namespace ue5parse {

namespace {

int hexValue(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

constexpr std::size_t blockSize = 16;

void decryptEcb(const unsigned char* key, unsigned int bits, std::span<std::byte> data) {
    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_dec(&context, key, bits);
    auto* bytes = reinterpret_cast<unsigned char*>(data.data());
    for (std::size_t offset = 0; offset + blockSize <= data.size(); offset += blockSize)
        mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_DECRYPT, bytes + offset, bytes + offset);
    mbedtls_aes_free(&context);
}

std::uint32_t loadUint32(std::span<const std::byte> data, std::size_t offset) {
    std::uint32_t value;
    std::memcpy(&value, data.data() + offset, 4);
    return value;
}

void storeUint32(std::span<std::byte> data, std::size_t offset, std::uint32_t value) {
    std::memcpy(data.data() + offset, &value, 4);
}

bool isIndexPart(EncryptedPart part) {
    return part == EncryptedPart::PakIndex || part == EncryptedPart::PakDirectoryIndex || part == EncryptedPart::TocDirectoryIndex;
}

bool hasGameDecryption(EGame game) {
    switch (game) {
    case EGame::GAME_DreamStar:
    case EGame::GAME_InfinityNikki:
    case EGame::GAME_PromiseMascotAgency:
    case EGame::GAME_Snowbreak:
    case EGame::GAME_BloodBowl3:
    case EGame::GAME_NeedForSpeedMobile:
    case EGame::GAME_DragonSwordAwakening:
    case EGame::GAME_RocoKingdomWorld:
    case EGame::GAME_OnePieceAmbition:
    case EGame::GAME_ApexLegendsMobile:
    case EGame::GAME_MarvelRivals:
    case EGame::GAME_Undawn:
    case EGame::GAME_PaxDei:
    case EGame::GAME_3on3FreeStyleRebound:
    case EGame::GAME_FunkoFusion:
    case EGame::GAME_TonyHawkProSkater12:
    case EGame::GAME_TonyHawkProSkater34:
    case EGame::GAME_Spectre:
    case EGame::GAME_Splitgate2:
    case EGame::GAME_Empulse:
    case EGame::GAME_MindsEye:
    case EGame::GAME_UnchartedWatersOrigin:
    case EGame::GAME_AssaultFireFuture:
    case EGame::GAME_ArcRaiders:
    case EGame::GAME_Rennsport:
    case EGame::GAME_ArenaBreakoutInfinite:
    case EGame::GAME_eBaseballProSpirit:
        return true;
    default:
        return false;
    }
}

//! Promise Mascot Agency replaces AES with two TEA-like rounds per 8-byte half of a block, keyed by the first 16 key bytes.
void decryptPromiseMascotAgency(const AesKey& key, std::span<std::byte> data) {
    std::span<const std::byte> keyBytes{key};
    std::uint32_t v5 = loadUint32(keyBytes, 8);
    std::uint32_t v6 = loadUint32(keyBytes, 12);
    std::uint32_t v7 = loadUint32(keyBytes, 4);
    std::uint32_t v9 = loadUint32(keyBytes, 0);
    for (std::size_t offset = 0; offset + blockSize <= data.size(); offset += blockSize) {
        std::uint32_t v10 = loadUint32(data, offset + 4);
        std::uint32_t v11 = loadUint32(data, offset);
        v10 -= (v11 + 0x5CA01372) ^ (v5 + 16 * v11) ^ (v6 + (v11 >> 5));
        v11 -= (v10 + 0x5CA01372) ^ (v7 + (v10 >> 5)) ^ (v9 + 16 * v10);
        std::uint32_t v12 = v10 - ((v11 - 0x51AFF647) ^ (v6 + (v11 >> 5)) ^ (v5 + 16 * v11));
        v11 -= (v12 - 0x51AFF647) ^ (v7 + (v12 >> 5)) ^ (v9 + 16 * v12);
        std::uint32_t v13 = loadUint32(data, offset + 8);
        std::uint32_t v14 = loadUint32(data, offset + 12);
        v14 -= (v13 + 0x5CA01372) ^ (v5 + 16 * v13) ^ (v6 + (v13 >> 5));
        std::uint32_t v15 = v13 - ((v14 + 0x5CA01372) ^ (v7 + (v14 >> 5)) ^ (v9 + 16 * v14));
        std::uint32_t v16 = v14 - ((v15 - 0x51AFF647) ^ (v5 + 16 * v15) ^ (v6 + (v15 >> 5)));
        std::uint32_t result = v16 - 0x51AFF647;
        storeUint32(data, offset, v11);
        storeUint32(data, offset + 4, v12);
        storeUint32(data, offset + 8, v15 - (result ^ (v9 + 16 * v16) ^ (v7 + (v16 >> 5))));
        storeUint32(data, offset + 12, v16);
    }
}

//! Snowbreak's key per archive: the user key encrypts the lowercase hex MD5 of the lowercase archive name without extension.
AesKey getSnowbreakKey(const AesKey& key, std::string_view archiveName) {
    std::size_t slash = archiveName.find_last_of("/\\");
    if (slash != std::string_view::npos)
        archiveName.remove_prefix(slash + 1);
    std::size_t dot = archiveName.rfind('.');
    if (dot != std::string_view::npos)
        archiveName = archiveName.substr(0, dot);
    std::string name(archiveName);
    std::ranges::transform(name, name.begin(), [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
    unsigned char digest[16];
    mbedtls_md5(reinterpret_cast<const unsigned char*>(name.data()), name.size(), digest);
    constexpr char hex[] = "0123456789abcdef";
    AesKey derived{};
    for (std::size_t i = 0; i < 16; i++) {
        derived[i * 2] = static_cast<std::byte>(hex[digest[i] >> 4]);
        derived[i * 2 + 1] = static_cast<std::byte>(hex[digest[i] & 0xF]);
    }
    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_enc(&context, reinterpret_cast<const unsigned char*>(key.data()), 256);
    auto* bytes = reinterpret_cast<unsigned char*>(derived.data());
    mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, bytes, bytes);
    mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, bytes + blockSize, bytes + blockSize);
    mbedtls_aes_free(&context);
    return derived;
}

//! Roco Kingdom World swaps bit pairs, bit quads and nibbles of each stored byte and reverses all key bytes but the last.
std::byte permuteRocoKingdomWorldByte(std::byte stored) {
    auto value = static_cast<std::uint8_t>(stored);
    auto stage1 = static_cast<std::uint8_t>(((value & 0xD5) << 1) | ((value >> 1) & 0x55));
    auto stage2 = static_cast<std::uint8_t>(((stage1 & 0xF3) << 2) | ((stage1 >> 2) & 0x33));
    return static_cast<std::byte>(static_cast<std::uint8_t>((stage2 >> 4) | (stage2 << 4)));
}

AesKey mutateRocoKingdomWorldKey(const AesKey& key) {
    AesKey mutated{};
    for (std::size_t i = 0; i + 1 < key.size(); i++)
        mutated[i] = key[key.size() - i - 2];
    mutated.back() = key.back();
    return mutated;
}

}

void decryptAesEcb(const AesKey& key, std::span<std::byte> data) {
    decryptEcb(reinterpret_cast<const unsigned char*>(key.data()), 256, data);
}

Result<AesKey> parseAesKey(std::string_view text) {
    if (text.starts_with("0x") || text.starts_with("0X"))
        text.remove_prefix(2);
    if (text.size() != 64)
        return fail(ErrorCode::BadAesKey, "parseAesKey.length");
    AesKey key{};
    for (std::size_t i = 0; i < key.size(); i++) {
        int high = hexValue(text[i * 2]);
        int low = hexValue(text[i * 2 + 1]);
        if (high < 0 || low < 0)
            return fail(ErrorCode::BadAesKey, "parseAesKey.digit");
        key[i] = static_cast<std::byte>(high * 16 + low);
    }
    return key;
}

struct AesDecryptor::Contexts {
    std::vector<mbedtls_aes_context> decrypt;
    std::vector<mbedtls_aes_context> encrypt;

    ~Contexts() {
        for (mbedtls_aes_context& context : decrypt)
            mbedtls_aes_free(&context);
        for (mbedtls_aes_context& context : encrypt)
            mbedtls_aes_free(&context);
    }
};

AesDecryptor::AesDecryptor(const KeyRing& keys) : guids(keys.guids), keys(keys.keys), contexts(std::make_unique<Contexts>()) {
    contexts->decrypt.resize(keys.keys.size());
    contexts->encrypt.resize(keys.keys.size());
    for (std::size_t i = 0; i < keys.keys.size(); i++) {
        const auto* key = reinterpret_cast<const unsigned char*>(keys.keys[i].data());
        mbedtls_aes_init(&contexts->decrypt[i]);
        mbedtls_aes_init(&contexts->encrypt[i]);
        mbedtls_aes_setkey_dec(&contexts->decrypt[i], key, 256);
        mbedtls_aes_setkey_enc(&contexts->encrypt[i], key, 256);
    }
}

AesDecryptor::~AesDecryptor() = default;

bool AesDecryptor::hasKey(FGuid keyGuid) const noexcept {
    for (const FGuid& guid : guids)
        if (guid == keyGuid)
            return true;
    return false;
}

Result<void> AesDecryptor::decryptGame(const DecryptRequest& request, std::size_t keyIndex) const noexcept {
    std::span<std::byte> data = request.data;
    if (data.size() % blockSize != 0)
        return fail(ErrorCode::InvalidData, "AesDecryptor.decrypt.ecbSize", request.offset);
    auto aes = [&] {
        const mbedtls_aes_context& context = contexts->decrypt[keyIndex];
        auto* bytes = reinterpret_cast<unsigned char*>(data.data());
        for (std::size_t offset = 0; offset < data.size(); offset += blockSize)
            mbedtls_aes_crypt_ecb(const_cast<mbedtls_aes_context*>(&context), MBEDTLS_AES_DECRYPT, bytes + offset, bytes + offset);
    };
    switch (request.game) {
    case EGame::GAME_OnePieceAmbition:
        //! One Piece Ambition does not encrypt the index; a file block is two XORed 8-byte halves.
        if (request.part != EncryptedPart::PakEntry)
            return {};
        for (std::size_t offset = 0; offset < data.size(); offset += blockSize) {
            std::uint64_t low;
            std::uint64_t high;
            std::memcpy(&low, data.data() + offset, 8);
            std::memcpy(&high, data.data() + offset + 8, 8);
            std::uint64_t first = high ^ 0xD6D6D6D6D6D6D6D6;
            std::uint64_t second = low ^ first;
            std::memcpy(data.data() + offset, &first, 8);
            std::memcpy(data.data() + offset + 8, &second, 8);
        }
        return {};
    case EGame::GAME_NeedForSpeedMobile:
        //! Need for Speed Mobile encrypts only the directory index of its IoStore containers.
        if (request.part != EncryptedPart::CompressionBlock)
            aes();
        return {};
    case EGame::GAME_DreamStar:
        aes();
        for (std::byte& value : data) {
            auto mixed = static_cast<std::uint8_t>(static_cast<std::uint8_t>(value) ^ 0x5F);
            value = static_cast<std::byte>(static_cast<std::uint8_t>((mixed << 5) | (mixed >> 3)));
        }
        return {};
    case EGame::GAME_InfinityNikki:
        aes();
        for (std::size_t offset = 0; offset < data.size(); offset += blockSize) {
            data[offset] ^= keys[keyIndex].back();
            data[offset + blockSize - 1] ^= keys[keyIndex].front();
        }
        return {};
    case EGame::GAME_PromiseMascotAgency:
        decryptPromiseMascotAgency(keys[keyIndex], data);
        return {};
    case EGame::GAME_Snowbreak: {
        if (std::ranges::all_of(keys[keyIndex], [](std::byte value) { return value == std::byte{0}; }))
            return {};
        AesKey derived = getSnowbreakKey(keys[keyIndex], request.archiveName);
        decryptAesEcb(derived, data);
        return {};
    }
    case EGame::GAME_BloodBowl3:
        //! Blood Bowl 3 uses AES-128 with the first half of the key.
        decryptEcb(reinterpret_cast<const unsigned char*>(keys[keyIndex].data()), 128, data);
        return {};
    case EGame::GAME_DragonSwordAwakening:
        aes();
        //! Dragon Sword XORs its index with the third decrypted byte.
        if (isIndexPart(request.part) && data.size() >= blockSize) {
            std::byte mask = data[2];
            for (std::size_t i = 0; i < data.size() / 8 * 8; i++)
                data[i] ^= mask;
        }
        return {};
    case EGame::GAME_RocoKingdomWorld: {
        for (std::byte& value : data)
            value = permuteRocoKingdomWorldByte(value);
        AesKey mutated = mutateRocoKingdomWorldKey(keys[keyIndex]);
        decryptAesEcb(mutated, data);
        return {};
    }
    case EGame::GAME_ApexLegendsMobile:
        decryptApexLegendsMobile(data, keys[keyIndex]);
        return {};
    case EGame::GAME_MarvelRivals:
        //! Marvel Rivals uses its own AES only in pak files and plain AES in IoStore containers.
        if (request.part != EncryptedPart::TocDirectoryIndex && request.part != EncryptedPart::CompressionBlock)
            decryptMarvelRivals(data, keys[keyIndex]);
        else
            aes();
        return {};
    case EGame::GAME_Undawn:
        decryptUndawn(data, keys[keyIndex]);
        return {};
    case EGame::GAME_PaxDei:
        decryptPaxDei(data, keys[keyIndex]);
        return {};
    case EGame::GAME_3on3FreeStyleRebound:
        decryptFreeStyleRebound(data);
        return {};
    case EGame::GAME_FunkoFusion:
        decryptFunkoFusion(data);
        return {};
    case EGame::GAME_TonyHawkProSkater12:
    case EGame::GAME_TonyHawkProSkater34:
        decryptTonyHawkProSkater12(data);
        return {};
    case EGame::GAME_Spectre:
        decryptSpectre(data, keys[keyIndex]);
        return {};
    case EGame::GAME_Splitgate2:
    case EGame::GAME_Empulse:
        decryptSplitgate2(data, keys[keyIndex]);
        return {};
    case EGame::GAME_MindsEye:
        decryptMindsEye(data, keys[keyIndex], isIndexPart(request.part));
        return {};
    case EGame::GAME_UnchartedWatersOrigin:
        decryptUnchartedWatersOrigin(data, keys[keyIndex]);
        return {};
    case EGame::GAME_AssaultFireFuture:
        decryptAssaultFireFuture(data);
        return {};
    case EGame::GAME_ArcRaiders:
        decryptArcRaiders(data, keys[keyIndex], isIndexPart(request.part));
        return {};
    case EGame::GAME_ArenaBreakoutInfinite:
        decryptArenaBreakoutInfinite(data, keys[keyIndex], isIndexPart(request.part));
        return {};
    case EGame::GAME_Rennsport:
        decryptRennsport(data, keys[keyIndex], request.part, request.chain);
        return {};
    case EGame::GAME_eBaseballProSpirit:
        if (!decryptProSpi(data, keys[keyIndex]))
            return fail(ErrorCode::InvalidData, "AesDecryptor.decrypt.proSpiDescriptor", request.offset);
        return {};
    default:
        aes();
        return {};
    }
}

Result<void> AesDecryptor::decrypt(const DecryptRequest& request) const noexcept {
    if (needsNoKey(request.game))
        return decryptGame(request, 0);
    std::size_t keyIndex = guids.size();
    for (std::size_t i = 0; i < guids.size(); i++)
        if (guids[i] == request.keyGuid)
            keyIndex = i;
    if (keyIndex == guids.size())
        return fail(ErrorCode::MissingKey, "AesDecryptor.decrypt", request.offset);
    if (request.method == EncryptionMethod::AesEcb && hasGameDecryption(request.game))
        return decryptGame(request, keyIndex);

    auto* data = reinterpret_cast<unsigned char*>(request.data.data());
    std::size_t size = request.data.size();
    if (request.method == EncryptionMethod::AesEcb) {
        if (size % blockSize != 0)
            return fail(ErrorCode::InvalidData, "AesDecryptor.decrypt.ecbSize", request.offset);
        const mbedtls_aes_context& context = contexts->decrypt[keyIndex];
        for (std::size_t offset = 0; offset < size; offset += blockSize)
            mbedtls_aes_crypt_ecb(const_cast<mbedtls_aes_context*>(&context), MBEDTLS_AES_DECRYPT, data + offset, data + offset);
        return {};
    }
    if (request.method == EncryptionMethod::AesCtr) {
        //! UE's AES-CTR counter block: the 12-byte IV, then a big-endian 32-bit block counter from zero.
        const mbedtls_aes_context& context = contexts->encrypt[keyIndex];
        unsigned char counter[blockSize];
        unsigned char stream[blockSize];
        std::memcpy(counter, request.iv.data(), 12);
        std::uint32_t block = 0;
        for (std::size_t offset = 0; offset < size; offset += blockSize) {
            counter[12] = static_cast<unsigned char>(block >> 24);
            counter[13] = static_cast<unsigned char>(block >> 16);
            counter[14] = static_cast<unsigned char>(block >> 8);
            counter[15] = static_cast<unsigned char>(block);
            mbedtls_aes_crypt_ecb(const_cast<mbedtls_aes_context*>(&context), MBEDTLS_AES_ENCRYPT, counter, stream);
            std::size_t count = size - offset < blockSize ? size - offset : blockSize;
            for (std::size_t i = 0; i < count; i++)
                data[offset + i] ^= stream[i];
            block++;
        }
        return {};
    }
    return {};
}

}
