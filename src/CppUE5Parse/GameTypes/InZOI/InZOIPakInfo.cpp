#include "CppUE5Parse/GameTypes/InZOI/InZOIPakInfo.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

#include <mbedtls/asn1.h>
#include <mbedtls/base64.h>
#include <mbedtls/bignum.h>

namespace ue5parse {

namespace {

constexpr std::size_t rsaBlockSize = 256;
constexpr std::size_t clearPrefixSize = inZOIEncryptedFooterSize - rsaBlockSize;

class Mpi {
public:
    Mpi() { mbedtls_mpi_init(&value); }
    Mpi(const Mpi&) = delete;
    Mpi& operator=(const Mpi&) = delete;
    ~Mpi() { mbedtls_mpi_free(&value); }
    mbedtls_mpi* get() { return &value; }

private:
    mbedtls_mpi value;
};

Result<std::vector<unsigned char>> decodePem(std::span<const std::byte> pem, bool& rsaKeyOnly) {
    std::string_view text(reinterpret_cast<const char*>(pem.data()), pem.size());
    std::size_t begin = text.find("-----BEGIN ");
    if (begin == std::string_view::npos)
        return fail(ErrorCode::InvalidData, "decryptInZOIPakInfo.pem");
    std::size_t headerEnd = text.find("-----", begin + 11);
    if (headerEnd == std::string_view::npos)
        return fail(ErrorCode::InvalidData, "decryptInZOIPakInfo.pem");
    rsaKeyOnly = text.substr(begin + 11, headerEnd - begin - 11) == "RSA PUBLIC KEY";
    std::size_t end = text.find("-----END ", headerEnd + 5);
    if (end == std::string_view::npos)
        return fail(ErrorCode::InvalidData, "decryptInZOIPakInfo.pem");
    std::vector<unsigned char> base64;
    for (char c : text.substr(headerEnd + 5, end - headerEnd - 5))
        if (c != '\r' && c != '\n' && c != ' ' && c != '\t')
            base64.push_back(static_cast<unsigned char>(c));
    std::size_t size = 0;
    mbedtls_base64_decode(nullptr, 0, &size, base64.data(), base64.size());
    std::vector<unsigned char> der(size);
    if (mbedtls_base64_decode(der.data(), der.size(), &size, base64.data(), base64.size()) != 0)
        return fail(ErrorCode::InvalidData, "decryptInZOIPakInfo.base64");
    der.resize(size);
    return der;
}

bool readPublicKey(std::vector<unsigned char>& der, bool rsaKeyOnly, mbedtls_mpi* modulus, mbedtls_mpi* exponent) {
    unsigned char* cursor = der.data();
    unsigned char* end = cursor + der.size();
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0)
        return false;
    end = cursor + length;
    if (!rsaKeyOnly) {
        mbedtls_asn1_buf algorithm{};
        mbedtls_asn1_buf parameters{};
        if (mbedtls_asn1_get_alg(&cursor, end, &algorithm, &parameters) != 0 || mbedtls_asn1_get_bitstring_null(&cursor, end, &length) != 0)
            return false;
        end = cursor + length;
        if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0)
            return false;
    }
    return mbedtls_asn1_get_mpi(&cursor, end, modulus) == 0 && mbedtls_asn1_get_mpi(&cursor, end, exponent) == 0;
}

}

Result<void> decryptInZOIPakInfo(std::span<std::byte> tail, std::span<const std::byte> publicKeyPem) {
    std::int32_t last = 0;
    if (tail.size() < inZOIEncryptedFooterSize)
        return fail(ErrorCode::InvalidData, "decryptInZOIPakInfo.size");
    std::memcpy(&last, tail.data() + tail.size() - sizeof(last), sizeof(last));
    if (last == 0)
        return {};
    if (publicKeyPem.empty())
        return fail(ErrorCode::MissingKey, "decryptInZOIPakInfo.publicKey");
    bool rsaKeyOnly = false;
    Result<std::vector<unsigned char>> der = decodePem(publicKeyPem, rsaKeyOnly);
    if (!der)
        return std::unexpected(der.error());
    Mpi modulus;
    Mpi exponent;
    if (!readPublicKey(*der, rsaKeyOnly, modulus.get(), exponent.get()) || mbedtls_mpi_size(modulus.get()) != rsaBlockSize)
        return fail(ErrorCode::InvalidData, "decryptInZOIPakInfo.publicKey");

    std::span<std::byte> encrypted = tail.last(rsaBlockSize);
    Mpi input;
    Mpi output;
    std::array<unsigned char, rsaBlockSize> block{};
    if (mbedtls_mpi_read_binary(input.get(), reinterpret_cast<const unsigned char*>(encrypted.data()), rsaBlockSize) != 0
        || mbedtls_mpi_cmp_mpi(input.get(), modulus.get()) >= 0
        || mbedtls_mpi_exp_mod(output.get(), input.get(), exponent.get(), modulus.get(), nullptr) != 0
        || mbedtls_mpi_write_binary(output.get(), block.data(), block.size()) != 0)
        return fail(ErrorCode::BadAesKey, "decryptInZOIPakInfo.rsa");

    //! PKCS #1 v1.5 block type 1: 00 01, at least eight FF bytes, 00, then the message.
    if (block[0] != 0 || block[1] != 1)
        return fail(ErrorCode::BadAesKey, "decryptInZOIPakInfo.padding");
    std::size_t separator = 2;
    while (separator < block.size() && block[separator] == 0xFF)
        separator++;
    if (separator == block.size() || block[separator] != 0 || separator < 10)
        return fail(ErrorCode::BadAesKey, "decryptInZOIPakInfo.padding");
    std::size_t messageSize = block.size() - separator - 1;

    std::array<std::byte, inZOIEncryptedFooterSize> combined{};
    std::memcpy(combined.data(), tail.data() + tail.size() - inZOIEncryptedFooterSize, clearPrefixSize);
    std::memcpy(combined.data() + clearPrefixSize, block.data() + separator + 1, messageSize);
    std::size_t combinedSize = clearPrefixSize + messageSize;
    std::memcpy(tail.data() + tail.size() - combinedSize, combined.data(), combinedSize);
    return {};
}

}
