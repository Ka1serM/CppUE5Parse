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
};

class Decryptor {
public:
    virtual ~Decryptor() = default;
    virtual bool hasKey(FGuid keyGuid) const noexcept = 0;
    virtual Result<void> decrypt(const DecryptRequest& request) const noexcept = 0;
};

using AesKey = std::array<std::byte, 32>;

Result<AesKey> parseAesKey(std::string_view text);

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
    std::vector<FGuid> guids;
    std::unique_ptr<Contexts> contexts;
};

}
