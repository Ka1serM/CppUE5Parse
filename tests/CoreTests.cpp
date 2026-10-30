#include <lz4.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "IoStoreWriter.h"
#include "Test.h"
#include "ue5parse/compress/Decompressor.h"
#include "ue5parse/core/ByteReader.h"
#include "ue5parse/core/FName.h"
#include "ue5parse/core/Text.h"
#include "ue5parse/crypto/Decryptor.h"

using namespace ue5parse;

namespace {

std::byte hexByte(const char* text) {
    auto digit = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
    return static_cast<std::byte>(digit(text[0]) * 16 + digit(text[1]));
}

test::Bytes fromHex(const char* text) {
    test::Bytes bytes(std::strlen(text) / 2);
    for (std::size_t i = 0; i < bytes.size(); i++) bytes[i] = hexByte(text + i * 2);
    return bytes;
}

}

TEST(nameNoneHashMatchesHashName) { CHECK(hashName("None") == FName::noneHash); }

TEST(nameHashAtCompileTimeMatchesRunTime) {
    // Every CityHash64 length class: 0-3, 4-7, 8-16, 17-32, 33-64, and over 64 with one and several 64-byte blocks.
    std::string text;
    for (std::size_t length = 0; length <= 300; length++) {
        CHECK(namehash::cityHash64Lower(text) == hashNameRuntime(text));
        text.push_back(static_cast<char>(length % 2 == 0 ? 'A' + length % 26 : 'a' + (length * 7) % 26));
    }
    static constexpr FName constant{"NotifyStateClass_2"};
    static_assert(constant.getNumber() == 3);
    CHECK(constant == FName(std::string("notifystateclass_2")));
}

TEST(nameIgnoresCase) {
    CHECK(FName("StaticMesh") == FName("staticmesh"));
    CHECK(!(FName("StaticMesh") == FName("StaticMesh_1")));
}

TEST(nameSplitsNumberSuffixAsUnreal) {
    FName mesh("Mesh_3");
    CHECK(mesh.getText() == "Mesh");
    CHECK(mesh.getNumber() == 4);
    FName zero("Mesh_0");
    CHECK(zero.getText() == "Mesh");
    CHECK(zero.getNumber() == 1);
    // A leading zero is part of the text.
    FName padded("Mesh_03");
    CHECK(padded.getText() == "Mesh_03");
    CHECK(padded.getNumber() == 0);
    FName plain("LZ4");
    CHECK(plain.getText() == "LZ4");
    CHECK(plain.getNumber() == 0);
}

TEST(byteReaderErrorIsSticky) {
    std::byte data[6] = {};
    ByteReader reader{data};
    reader.read<std::uint32_t>();
    CHECK(!reader.hasFailed());
    reader.read<std::uint32_t>();
    CHECK(reader.hasFailed());
    CHECK(reader.getError().code == ErrorCode::ReadPastEnd);
    CHECK(reader.read<std::uint16_t>() == 0);
    CHECK(reader.getPosition() == 4);
}

TEST(byteReaderRejectsLargeCounts) {
    std::int32_t count = 1000;
    std::byte data[16] = {};
    std::memcpy(data, &count, 4);
    ByteReader reader{data};
    CHECK(reader.readCount(4) == 0);
    CHECK(reader.getError().code == ErrorCode::ReadPastEnd);
}

TEST(utf16ToUtf8HandlesSurrogates) {
    // "aé€😀" in UTF-16 little endian.
    test::Bytes utf16 = fromHex("6100e900ac203dd800de");
    Arena arena;
    CHECK(utf16ToUtf8(utf16, arena) == "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
}

TEST(aesKeyParsing) {
    CHECK(parseAesKey("0x000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f").has_value());
    CHECK(parseAesKey("000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F").has_value());
    CHECK(parseAesKey("0x0001").error().code == ErrorCode::BadAesKey);
    CHECK(!parseAesKey("0x000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1g").has_value());
}

// FIPS-197 appendix C.3: AES-256.
TEST(aesEcbMatchesFips197) {
    Result<AesKey> key = parseAesKey("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    REQUIRE(key.has_value());
    KeyRing keys;
    keys.add(FGuid{}, *key);
    AesDecryptor decryptor{keys};
    test::Bytes data = fromHex("8ea2b7ca516745bfeafc49904b49608900112233445566778899aabbccddeeff");
    test::Bytes second = fromHex("00112233445566778899aabbccddeeff");
    test::encryptEcb(*key, std::span(data).subspan(16));
    DecryptRequest request;
    request.data = data;
    REQUIRE(decryptor.decrypt(request).has_value());
    CHECK(std::memcmp(data.data(), fromHex("00112233445566778899aabbccddeeff").data(), 16) == 0);
    CHECK(std::memcmp(data.data() + 16, second.data(), 16) == 0);
}

TEST(aesUnknownGuidIsMissingKey) {
    KeyRing keys;
    keys.add(FGuid{}, AesKey{});
    AesDecryptor decryptor{keys};
    CHECK(decryptor.hasKey(FGuid{}));
    CHECK(!decryptor.hasKey(FGuid{1, 2, 3, 4}));
    test::Bytes data(16);
    DecryptRequest request;
    request.data = data;
    request.keyGuid = FGuid{1, 2, 3, 4};
    CHECK(decryptor.decrypt(request).error().code == ErrorCode::MissingKey);
}

TEST(codecsDecompressZlibAndLz4) {
    Codecs codecs;
    test::Bytes data = test::makeData(10000, 7, true);
    test::Bytes zlib = test::compressZlib(data);
    test::Bytes output(data.size());
    const Decompressor* zlibCodec = codecs.find("zlib");
    REQUIRE(zlibCodec != nullptr);
    CHECK(zlibCodec->decompress(zlib, output).has_value());
    CHECK(output == data);

    test::Bytes lz4(LZ4_compressBound(static_cast<int>(data.size())));
    int size = LZ4_compress_default(reinterpret_cast<const char*>(data.data()), reinterpret_cast<char*>(lz4.data()),
                                    static_cast<int>(data.size()), static_cast<int>(lz4.size()));
    lz4.resize(static_cast<std::size_t>(size));
    std::fill(output.begin(), output.end(), std::byte{0});
    REQUIRE(codecs.find("LZ4") != nullptr);
    CHECK(codecs.find("LZ4")->decompress(lz4, output).has_value());
    CHECK(output == data);

    // A wrong size is an error, not a short result.
    test::Bytes larger(data.size() + 1);
    CHECK(zlibCodec->decompress(zlib, larger).error().code == ErrorCode::Decompression);
    CHECK(codecs.find("Unknown") == nullptr);
}
