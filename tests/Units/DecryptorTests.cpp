#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <string_view>

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

struct GameVector {
    EGame game;
    EncryptedPart part;
    std::string_view expected;
};

//! Outputs of the CUE4Parse decryptors for the key bytes i * 13 + 1 and the data bytes i * 7 + 3.
constexpr GameVector gameVectors[] = {
    {EGame::GAME_ApexLegendsMobile, EncryptedPart::PakEntry, "E75222E42B11806EF261A0DB108E9AA98EDA1C3CB1580667FBD925AA9598B8ECA64173ED6BFD521C8B37215E2BBAC2D65754AD2A8012B3B49B45FFFF5C93C3D6"},
    {EGame::GAME_ApexLegendsMobile, EncryptedPart::PakIndex, "E75222E42B11806EF261A0DB108E9AA98EDA1C3CB1580667FBD925AA9598B8ECA64173ED6BFD521C8B37215E2BBAC2D65754AD2A8012B3B49B45FFFF5C93C3D6"},
    {EGame::GAME_MarvelRivals, EncryptedPart::PakEntry, "7D19A1BF615062D0C3E9129D4412F32928E40D08E9A3BE37B15464644F7C70480F9D75846D23C7592CB11F7802749D2473762A5D252E39DB70DB6C786BAFB9EC"},
    {EGame::GAME_MarvelRivals, EncryptedPart::PakIndex, "7D19A1BF615062D0C3E9129D4412F32928E40D08E9A3BE37B15464644F7C70480F9D75846D23C7592CB11F7802749D2473762A5D252E39DB70DB6C786BAFB9EC"},
    {EGame::GAME_Undawn, EncryptedPart::PakEntry, "00C2807F1B7BCC3BCB05EBD0887500AC2925FC8E63FB7D794FD9D27641BA7D6D6D24C2B27CE659DD86D36AF3F1E18F4BDDCB703D415234288DF0D17E1666D554"},
    {EGame::GAME_Undawn, EncryptedPart::PakIndex, "00C2807F1B7BCC3BCB05EBD0887500AC2925FC8E63FB7D794FD9D27641BA7D6D6D24C2B27CE659DD86D36AF3F1E18F4BDDCB703D415234288DF0D17E1666D554"},
    {EGame::GAME_PaxDei, EncryptedPart::PakEntry, "AF4F6EB7532349A627062A893E066E78AA8786F4DED840765E75892214CC3E1FEB044CA0E4C7038018FB6A6BE5F92D6676AD7F6AD9ABB70FFB0127E03C6A8807"},
    {EGame::GAME_PaxDei, EncryptedPart::PakIndex, "AF4F6EB7532349A627062A893E066E78AA8786F4DED840765E75892214CC3E1FEB044CA0E4C7038018FB6A6BE5F92D6676AD7F6AD9ABB70FFB0127E03C6A8807"},
    {EGame::GAME_3on3FreeStyleRebound, EncryptedPart::PakEntry, "5B5D95329F7C9CEEC7BF1443597C69CFA7E25861028D5F3453FD4E918CB246C4094EC51620C1A9F8E09FF791B65C9EF857ED7B752A41AE21D2422C9998C6B1B9"},
    {EGame::GAME_3on3FreeStyleRebound, EncryptedPart::PakIndex, "5B5D95329F7C9CEEC7BF1443597C69CFA7E25861028D5F3453FD4E918CB246C4094EC51620C1A9F8E09FF791B65C9EF857ED7B752A41AE21D2422C9998C6B1B9"},
    {EGame::GAME_FunkoFusion, EncryptedPart::PakEntry, "3D8ED4F2984038E944034023F915406C13840AB2581D799ADC83B622B241D9B019C7AC1FAE802A170D43D6D3A04C843BBCFEBC2D59A716FB22DBC20D7F596825"},
    {EGame::GAME_FunkoFusion, EncryptedPart::PakIndex, "3D8ED4F2984038E944034023F915406C13840AB2581D799ADC83B622B241D9B019C7AC1FAE802A170D43D6D3A04C843BBCFEBC2D59A716FB22DBC20D7F596825"},
    {EGame::GAME_TonyHawkProSkater12, EncryptedPart::PakEntry, "496E72102D7ACFB6BD755950538195282011D78B232893C2F536CE799217747F7DA67ED30CA4C6048FE4875EBA26392A8877B315AECC007B7B2D57EC1D2B717A"},
    {EGame::GAME_TonyHawkProSkater12, EncryptedPart::PakIndex, "496E72102D7ACFB6BD755950538195282011D78B232893C2F536CE799217747F7DA67ED30CA4C6048FE4875EBA26392A8877B315AECC007B7B2D57EC1D2B717A"},
    {EGame::GAME_Spectre, EncryptedPart::PakEntry, "07FC81E27322F38021808AFB49117E62EBD60BF104DB22F3CBED0ADB60543DEA4CA4096332E1F63871D06117C14143E7A549D51A38B7BBF1AB7A201DD1E3A218"},
    {EGame::GAME_Spectre, EncryptedPart::PakIndex, "07FC81E27322F38021808AFB49117E62EBD60BF104DB22F3CBED0ADB60543DEA4CA4096332E1F63871D06117C14143E7A549D51A38B7BBF1AB7A201DD1E3A218"},
    {EGame::GAME_Splitgate2, EncryptedPart::PakEntry, "ACAC6017D21515D47FE6AA1C6567A464618CFAD10E9073E7C37B713089FDA9950EBCE6012B2F6DFF100A48EFD7992E7C2271AAB35B8DAEBEE0A08F92BF1081DF"},
    {EGame::GAME_Splitgate2, EncryptedPart::PakIndex, "ACAC6017D21515D47FE6AA1C6567A464618CFAD10E9073E7C37B713089FDA9950EBCE6012B2F6DFF100A48EFD7992E7C2271AAB35B8DAEBEE0A08F92BF1081DF"},
    {EGame::GAME_MindsEye, EncryptedPart::PakEntry, "492D958B556456E4F7DD26A97026C71D1CD0393CDD978A03856050507B48447C3BA941B05917F36D18852B4C3640A91047421E69111A0DEF44EF584C5F9B8DD8"},
    {EGame::GAME_MindsEye, EncryptedPart::PakIndex, "7F64B81EDE3132B2132AFB8FD956E1DA01CCE905E14A1D8986E530002B330C384792E8F1E94EE49E759DAE677A76E9B957055C77780B17E2ABABB71413C41655"},
    {EGame::GAME_UnchartedWatersOrigin, EncryptedPart::PakEntry, "3C356FD4940AC97C303EA7FDE1B75CFEAC1EB7D48A3884338BFBFBB2ADE132F305E319442F0F78B95C9608F6A08384E882693003798216F06B4A0304A3DE3B67"},
    {EGame::GAME_UnchartedWatersOrigin, EncryptedPart::PakIndex, "3C356FD4940AC97C303EA7FDE1B75CFEAC1EB7D48A3884338BFBFBB2ADE132F305E319442F0F78B95C9608F6A08384E882693003798216F06B4A0304A3DE3B67"},
    {EGame::GAME_AssaultFireFuture, EncryptedPart::PakEntry, "7B0790B4F22CF530D552FA6DFD7BEA60BCEE3A03561A105B20B9E1C9D22FFBCF1598C12181237B81FA6417255842D6A4F7459C3D5C55F44BDE7E269440EB009F"},
    {EGame::GAME_AssaultFireFuture, EncryptedPart::PakIndex, "7B0790B4F22CF530D552FA6DFD7BEA60BCEE3A03561A105B20B9E1C9D22FFBCF1598C12181237B81FA6417255842D6A4F7459C3D5C55F44BDE7E269440EB009F"},
    {EGame::GAME_ArcRaiders, EncryptedPart::PakEntry, "9247A8E9B5D3694C02B7781E63E98E08C1EAE9C1A0B5B4CD01A657C06D1E9FEEBB7A36F7D053C7C656E25AE1A68AD054D14D300551FA81A4B3265C0CD79D417C"},
    {EGame::GAME_ArcRaiders, EncryptedPart::PakIndex, "97CAA6FDE86F6B7C8302CC18B3F5DA14F73D74BE3FD664F10874ACFF40EE3BD26047F3B54C4EA35046DE3D22C3728C3695CDF41C7A31D074D7AFB079AAADF6A2"},
    {EGame::GAME_Rennsport, EncryptedPart::PakEntry, "8F6F2F590AEBF3C8752C4FEB7AACBF3431D54F83F6C184556B0247C232425CA2310C9144434461223B00E07DA19D7EE097688C4E026216C603DD87F2E4BAEE29"},
    {EGame::GAME_Rennsport, EncryptedPart::PakIndex, "1C7BB71631D3B0A467F7664395E81EF36A6138C4063D2BA4827728372F4998C4A4F98C0DF899429DC0D4B1D66DB6628300000000000000000000000000000000"},
    {EGame::GAME_Rennsport, EncryptedPart::PakDirectoryIndex, "E66EC40F05FBA556573D80C24017E3B6C6FEB31A7D22128AE5A2EA91DF9FEEB7A71132461827B85F03D619BDEAD4DD7F00000000000000000000000000000000"},
};

std::string toHex(std::span<const std::byte> data) {
    constexpr char digits[] = "0123456789ABCDEF";
    std::string text;
    for (std::byte value : data) {
        text.push_back(digits[static_cast<std::uint8_t>(value) >> 4]);
        text.push_back(digits[static_cast<std::uint8_t>(value) & 0xF]);
    }
    return text;
}

bool decryptVector(const GameVector& vector) {
    AesKey key{};
    for (std::size_t i = 0; i < key.size(); i++)
        key[i] = static_cast<std::byte>(i * 13 + 1);
    std::array<std::byte, 64> data{};
    for (std::size_t i = 0; i < data.size(); i++)
        data[i] = static_cast<std::byte>(i * 7 + 3);
    if (!decryptAs(vector.game, key, data, vector.part))
        return false;
    std::string actual = toHex(data);
    if (actual != vector.expected)
        std::fprintf(stderr, "  game %d part %d: %s\n", static_cast<int>(vector.game), static_cast<int>(vector.part), actual.c_str());
    return actual == vector.expected;
}

}

TEST(customGameDecryptorsMatchCue4Parse) {
    for (const GameVector& vector : gameVectors)
        CHECK(decryptVector(vector));
}

TEST(rennsportEntryChainsAcrossRequests) {
    AesKey key = makeKey();
    std::array<std::byte, 64> whole{};
    for (std::size_t i = 0; i < whole.size(); i++)
        whole[i] = static_cast<std::byte>(i * 7 + 3);
    std::array<std::byte, 64> split = whole;
    REQUIRE(decryptAs(EGame::GAME_Rennsport, key, whole, EncryptedPart::PakEntry));
    KeyRing ring;
    ring.add(FGuid{}, key);
    AesDecryptor decryptor(ring);
    DecryptRequest request;
    request.data = std::span<std::byte>(split).subspan(32);
    request.part = EncryptedPart::PakEntry;
    request.game = EGame::GAME_Rennsport;
    std::array<std::byte, 16> chain{};
    std::memcpy(chain.data(), split.data() + 16, 16);
    request.chain = chain;
    REQUIRE(decryptor.decrypt(request).has_value());
    CHECK(std::memcmp(split.data() + 32, whole.data() + 32, 32) == 0);
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
