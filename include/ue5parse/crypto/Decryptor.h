#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "ue5parse/core/FGuid.h"
#include "ue5parse/core/Result.h"

namespace ue5parse {

enum class EncryptedPart : std::uint8_t { TocDirectoryIndex, CompressionBlock, PakIndex, PakEntry };

// EIoEncryptionMethod. Pak files and old IoStore containers use ECB.
enum class EncryptionMethod : std::uint8_t { None = 0, AesEcb = 1, AesCtr = 2 };

struct DecryptRequest {
    std::span<std::byte> data;            // decrypted in place; a multiple of 16 bytes for ECB
    FGuid keyGuid;                        // from the TOC header or the pak footer
    EncryptionMethod method = EncryptionMethod::AesEcb;
    std::array<std::byte, 12> iv{};       // AES-CTR: the counter prefix of this block (UE 6.0)
    EncryptedPart part = EncryptedPart::CompressionBlock;
    std::uint64_t containerId = 0;        // FIoContainerId, for game-specific schemes
    std::uint64_t offset = 0;             // of data in the container file
};

// Decrypts data in place. Several threads call it at once.
class Decryptor {
public:
    virtual ~Decryptor() = default;
    virtual bool hasKey(FGuid keyGuid) const noexcept = 0;
    virtual Result<void> decrypt(const DecryptRequest& request) const noexcept = 0;
};

using AesKey = std::array<std::byte, 32>;

// "0x" + 64 hex digits, or 64 hex digits.
Result<AesKey> parseAesKey(std::string_view text);

// Generic keys: guid -> AES-256 key. The zero guid is the main key.
struct KeyRing {
    std::vector<FGuid> guids;
    std::vector<AesKey> keys;

    void add(FGuid guid, const AesKey& key) {
        guids.push_back(guid);
        keys.push_back(key);
    }
};

// The default decryptor: AES-256 (ECB, and CTR for UE 6.0) with the key of the guid. The key schedules are expanded
// once, here.
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
    std::vector<FGuid> guids;
    std::unique_ptr<Contexts> contexts;
};

}
