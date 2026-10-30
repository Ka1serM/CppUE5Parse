#include <mbedtls/aes.h>

#include <cstring>

#include "ue5parse/crypto/Decryptor.h"

namespace ue5parse {

namespace {

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

constexpr std::size_t blockSize = 16;

}

Result<AesKey> parseAesKey(std::string_view text) {
    if (text.starts_with("0x") || text.starts_with("0X")) text.remove_prefix(2);
    if (text.size() != 64) return fail(ErrorCode::BadAesKey, "parseAesKey.length");
    AesKey key{};
    for (std::size_t i = 0; i < key.size(); i++) {
        int high = hexValue(text[i * 2]);
        int low = hexValue(text[i * 2 + 1]);
        if (high < 0 || low < 0) return fail(ErrorCode::BadAesKey, "parseAesKey.digit");
        key[i] = static_cast<std::byte>(high * 16 + low);
    }
    return key;
}

// One decryption context (ECB) and one encryption context (the CTR key stream) for each key. mbedtls contexts are
// read-only after setup, so all threads share them.
struct AesDecryptor::Contexts {
    std::vector<mbedtls_aes_context> decrypt;
    std::vector<mbedtls_aes_context> encrypt;

    ~Contexts() {
        for (mbedtls_aes_context& context : decrypt) mbedtls_aes_free(&context);
        for (mbedtls_aes_context& context : encrypt) mbedtls_aes_free(&context);
    }
};

AesDecryptor::AesDecryptor(const KeyRing& keys) : guids(keys.guids), contexts(std::make_unique<Contexts>()) {
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
        if (guid == keyGuid) return true;
    return false;
}

Result<void> AesDecryptor::decrypt(const DecryptRequest& request) const noexcept {
    std::size_t keyIndex = guids.size();
    for (std::size_t i = 0; i < guids.size(); i++)
        if (guids[i] == request.keyGuid) keyIndex = i;
    if (keyIndex == guids.size()) return fail(ErrorCode::MissingKey, "AesDecryptor.decrypt", request.offset);

    auto* data = reinterpret_cast<unsigned char*>(request.data.data());
    std::size_t size = request.data.size();
    if (request.method == EncryptionMethod::AesEcb) {
        if (size % blockSize != 0) return fail(ErrorCode::InvalidData, "AesDecryptor.decrypt.ecbSize", request.offset);
        const mbedtls_aes_context& context = contexts->decrypt[keyIndex];
        for (std::size_t offset = 0; offset < size; offset += blockSize)
            mbedtls_aes_crypt_ecb(const_cast<mbedtls_aes_context*>(&context), MBEDTLS_AES_DECRYPT, data + offset, data + offset);
        return {};
    }
    if (request.method == EncryptionMethod::AesCtr) {
        // UE's AES-CTR: the counter block is the 12-byte IV and a big-endian 32-bit block counter from zero.
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
            for (std::size_t i = 0; i < count; i++) data[offset + i] ^= stream[i];
            block++;
        }
        return {};
    }
    return {};
}

}
