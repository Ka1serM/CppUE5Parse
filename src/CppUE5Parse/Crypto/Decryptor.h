#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Core/FGuid.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"

namespace ue5parse {

enum class EncryptedPart : std::uint8_t { TocDirectoryIndex, CompressionBlock, PakIndex, PakEntry };

enum class EncryptionMethod : std::uint8_t { None = 0, AesEcb = 1, AesCtr = 2 };

struct DecryptRequest {
    std::span<std::byte> data;
    FGuid keyGuid;
    EncryptionMethod method = EncryptionMethod::AesEcb;
    std::array<std::byte, 12> iv{};
    EncryptedPart part = EncryptedPart::CompressionBlock;
    std::uint64_t containerId = 0;
    std::uint64_t offset = 0;
    EGame game = EGame::GAME_UE4_0;
    std::string_view archiveName;
};

class Decryptor {
public:
    virtual ~Decryptor() = default;
    virtual bool hasKey(FGuid keyGuid) const noexcept = 0;
    virtual Result<void> decrypt(const DecryptRequest& request) const noexcept = 0;
};

using AesKey = std::array<std::byte, 32>;

Result<AesKey> parseAesKey(std::string_view text);

//! Decrypts AES-256 ECB with a key that a game builds in.
void decryptAesEcb(const AesKey& key, std::span<std::byte> data);

//! Games whose encrypted data needs no AES key that the user gives.
constexpr bool needsNoKey(EGame game) {
    return game == EGame::GAME_OnePieceAmbition || game == EGame::GAME_TonyHawkProSkater12 || game == EGame::GAME_TonyHawkProSkater34;
}

struct KeyRing {
    std::vector<FGuid> guids;
    std::vector<AesKey> keys;

    void add(FGuid guid, const AesKey& key) {
        guids.push_back(guid);
        keys.push_back(key);
    }
};

class AesDecryptor final : public Decryptor {
public:
    explicit AesDecryptor(const KeyRing& keys);
    ~AesDecryptor() override;
    AesDecryptor(const AesDecryptor&) = delete;
    AesDecryptor& operator=(const AesDecryptor&) = delete;

    bool hasKey(FGuid keyGuid) const noexcept override;
    Result<void> decrypt(const DecryptRequest& request) const noexcept override;

private:
    struct Contexts;
    Result<void> decryptGame(const DecryptRequest& request, std::size_t keyIndex) const noexcept;

    std::vector<FGuid> guids;
    std::vector<AesKey> keys;
    std::unique_ptr<Contexts> contexts;
};

}
