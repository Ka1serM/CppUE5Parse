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
#include "CppUE5Parse/GameTypes/ProSpi/ProSpiDecrypt.h"
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
    {EGame::GAME_ArenaBreakoutInfinite, EncryptedPart::PakEntry, "9099828B8CB5BEA7A8D1DAC3C4CDF6FFE0E9121B1C050E3738212A53545D464F7079626B6C959E8788B1BAA3A4ADD6DFC0C9F2FBFCE5EE1718010A33343D262F"},
    {EGame::GAME_ArenaBreakoutInfinite, EncryptedPart::PakIndex, "4A5B72E361B39F725E9DB77212436CE672D149B67A7C954D3BC0C1B09DBCB20BDFAAB90ABE25FCC213E3B84C194B59A33CD99DCDE6315B948E150D6AC0918B17"},
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

namespace {

struct ProSpiVector {
    std::string_view trailer;
    std::string_view expected;
};

//! Made with the CUE4Parse ProSpi ciphers, one trailer for each known cipher descriptor.
constexpr std::array<ProSpiVector, 31> proSpiVectors = {{
    {"07A908FA0B6B6BE9A0DB624C4E5CBC218DAE11AD6808FA8D", "AC8658222B17F11B986A490C687E015FC0BBEF071773076293FBAA06FB331ED3701BADA7C10B13680DEFC8D4912B8669075853D4B93B08E4E95032707FA735BEA195CC9A8513236E"},
    {"0894FD8CB9D609E9AA1381D057BD101A64B2D316449AA146", "C6DCB3E6FBD22F3F4CFC43B244F505F152BE0929748C6DC17266FC710FE9DC2A4B02131B96BD255DAF79F7F647500153BB783B715D3B68B0F30AE10C1374CAB0FCE4317C0EAFBC9A"},
    {"3ED62FA31298F1D45B2319517B1F014D3F535151FF2A3242", "97AC45F6404295D20C2E8CA7D3B793E9FB958AFDC15C7E3120E7086D1AC3A497FB5A8764124D5550D667A10B96E2B0CFF4CDBF0D4D6F878F5B02062B4331CA663DAF70E46463BC09"},
    {"DD2A8C794B152F3D0BB364D8CD6EEA436D05CB68445A78FE", "42F771CFFB583E168100FC57512AE30AF526CAD0B71762FB1D025EA47E0DD8B65C0F032E389C312437B5F39118693740509E129276BD89768885AD0A0872F40FA8E1B6816F3780D4"},
    {"4C9EBDEE9FD05545810A3A1CD0CDCA0F41A35CE2415FD774", "2A73310CB69A8DDA8D7D2F190F801668F678D8D29F368967E482BF22AD2C112A0CE4B3E971E4FF3395902579ACA7D747D5FAB0EACB83B50E5D47D4E0C1E96322E9B451A246895A97"},
    {"9E8732C26FC53AD0AB8A399344E6C9D445B5EC47B544903D", "65C71D165243E35ED2C6E93E6297FE376255F24740F7951313B262A90FDBCFAE2885520460BCBDA41DAC11CF6CF014667175F663F313829E92BB62F4B7E7FBF76477DCC554716712"},
    {"DB90697363354BB5A82DF30812EA61633BDDE48FE80B4D6D", "EE5077B218B63DFEE5E36EC85ECB93CC09C16087AF5974AA1CD4EBF4195E575B9346C016BB3E2CEE0724E02ADCFDC9B597B6212576BA9602B4AC9BC99513661CB53D2E6F25842427"},
    {"5AC809A134EC41979E13A3864D450DFBE4574D6FF7588EEF", "C56A5699EAEBA7C2E112D26B6C4CBFDCF2CA583BE5B497BACC861D94905D3BA00C77C7EEB03404108983C0ABE8FB2A266A60A5DF2E77308AA5CF15D6CDF5C14FBAC82F275B3A4D3B"},
    {"1FB51EB8C710F497162E52D62D19C502DEFBA693782CAE2D", "FC81FEA679D57816C0AC07CEA29DD24AB57E72C0249EEBDCA4B56C89C9A1184BAF77026835863D0224670050C34B3441AFC9F26FE4AA9F55E558F75E3E926CDCF13906DE00C43A7B"},
    {"F0CF5040C4A3BA9192FF70300C7852C689D2ADF2A378D803", "E24961D14468C99828A91114DB52FCA9FBC46F0CB5F66ABC04E5C1280A30EB12F027CF27B6A7352B8FC5DDD5F6FB5B1F571F666B1F05AAEB8EE785119648FA1AEB899C07BB263647"},
    {"D633CD18142A0A5EAA335D1D955CEE6F2FBFAE87C8D1BA83", "E53E71F7D8A0B5709551D91C386BEDDACD27CB1367A71EEF16B26C631FB5DB691B7A31F17F8F2A1A7649DE09C6841774AD01BFA78CDA226552CA7284BE0B3D1312947420E15E071C"},
    {"A9FEC5DE095B5B014F6961C97A2808C67977D7FC893438C8", "3012B8D9770CBBFBB9653A14C82918C62194D65309CEF2C687A9DEA7A20BBA90E1A1B4B5BDB0B346EA72A259D8AB5A207FBE9A5077791832668FF85054ED80C309C588257F131945"},
    {"44C841AD146A08E43DA3D8C3EEB7C015838AB948A29D7A0C", "E092AA33274ED680D96F7FC3201D230103C445D4E6C328303C031E9C71A5097F899B13AE76070A5273D73461AA68673A082444302D59E22FDE70EAE3F18EC8DE79319846329AA970"},
    {"DED17C54927979CF9E358FBEBBBC216D35171F350B03D50E", "4AB6F3C57176628D4BC92D99E22C522B26B09E146DF2301BF3C0868CDECF7866A0131B3BCEB9591EBB34E4019A9056A6F9921BA506C4D0D415F35FF183E1F10A3569CFBF1D4125B8"},
    {"D2BF4ECAC9A407F746A63F7F25079F42EA1D4FCD6E60E417", "97ECD72254B1CF09366AD94E7C329B88719E94957C3EC7CB167E672750F720BF2C448AE914F270CB8DF0F716D58E4DB6D8D4F7DE0BE9AB378056BE007B9F0366B59765F6E35A4B05"},
    {"A7C0ED5442C275C36882CF1C183E06E8EBA4FD62285E22D6", "071CFD6C3F370B05CCC90DDDE2A00D3B2951377D153F67F1D6B65C2BEC045CC1A7C536B6820CC9C33C225E293F767E4FEE93F874E7591DDB474E0DBCE00D5A17E6A3F6995251742E"},
    {"D2A3781C3440C63B190D330103CD7016CEC7BA7D5F6BD310", "2496C534112D8D6E5497881DD1206CC5F47D1520465432A441152756DB3CA7AD2A0A9CA331D44A4C22CB384AAA7F6812186C7DBBAB9165E274812E27FFC4A7F7B792E5E244FA72FB"},
    {"70105090A5C8D50EB2A594F1BE7BEF2CFA4371F5F80569CD", "C120BCC34348B6D1D535DCC58430F10371904A4663131884EED8AA37A0F8B634A1C49093C8801017DF3B2D58EF6F625113A01A7E0AD2508A173877150BC06D320CE5E4D6B8DB8C15"},
    {"EE098128D035657117376D9DE24BB136166E4064CA971CC4", "B1E8FEF0A31DB238F7E09362AF284C72244DE401E9AEE8FCDC1796FCE8A5362EA681AE594081977D4D63CD2A69C28DB43B30CEEAEE8C15F7CCE96CCF3BFA06C1E13BE2AD9222B8F8"},
    {"311B05B70036C04DE5D499FC89C0310F79EFFCC1D4354D1B", "EACA735304624A92C9AEB9662CAE013FFAA4421780B82DC1FE5447A393DC13ADB7BBF3AF31D73118C9DFD762D819765B8200B889FBA412E605F510DFFA311E576BD47AA2DB572D60"},
    {"875CA60D855440F67777E398DB50C0312F772939016D5FCB", "C17717E974E84742C37E67C551C633E79CF3FBD7992BDD74B93F9DD132FB8525ED545F1E0D35F327DBECED55CCC1E519A05F693A96D27AFFDDC99D5EC02457C2D298BC5784B876CB"},
    {"467406F26D9F6AA855674FD170B6A97270727EE0797327FD", "556FA8E84E4222AEBF1BDF8815AB40DC627068F33BB88944D35802117F22B01918C80673640F07DEDFDACF7D710986EC0F49F2B17F84766EB6C125C5C11727C0690F9E18DDEBAC78"},
    {"AC558D04A451FB5466AECFD0A104A49D268A25715BA95058", "502C84B40499FDBE5D4C8A3CD4B90CFE0C636C12229CFB4B3D1D08CF88E1431B1F8BD3FD54D99372B5D41F7CFFEB9C839CCF3EDAA1E7AFFBC8FFFBCA836CE60E3CAE1DF15AF77905"},
    {"E6B920B8BBA8EFF0F154AEEB56065E74FA8D888964CAF5FE", "765B95565EFB05E074FCC4BE41AF50601BB2715652EDC71F45A2C97D84C03F676B805B49F5A21EEF3276C2F88B4CC1F81FB8ECB4A6D0512300EB523B2FC20358192214CC4A3289B5"},
    {"1E44FE716FF1D08D128E171FA78C1D53D75E528CD1142ABC", "0320497BC3830E7751DEC704CB2CF1E7A7ED678E149795B26D7160ED4BF34E2CD3F5CCB3F2D5B6DE38EBF367191A7798B8B85A61466003A9422D62707752BDEB44D5F2859649D505"},
    {"CB565C5470829974BEEBF06D63591F9B8493988986A5BE5B", "ECA6604DCC38C39BC34B694CBCB28F1A8FCF600A7254B5F8D0B0BD32080E5BA2F0DFD72DDA4E3066F87C5E667B553A241A547A64ECBDDD2D59F707255CD1C334F7F9996F3FBF5D05"},
    {"41278BF6CF5CBCAF6C7E34B6E9F60315079364C6E55452C2", "C92E8E943C21C886F602AB4D70E1E5F87FFEC0C00D56E78B28831A574506A1EE2DC5DECF47BE023B9FFA5207AFC101F537857C186DB68CB1EDC7AD4786119C9F764EC7A3D6DCB7E8"},
    {"968D7CC4E7F415631BC360BE4F7773FFBED3A00F914034C6", "651D9C5F21533CF85FAC0CE0BECFB3093EE72DBE09071BEDA18AB1009C40E40EA6C13BFA7759AECFD899B07318B766E880AB72C06A9F82FE8F7F796D350718D8ED9751880621B874"},
    {"623E2E7422780A6A00AE21E7FCAD2492F3E6A53460903F9D", "1ECD4F6A65135A4D6B2869BF7FFD55F815DDFC900DE18DF62064775E0231133B29E8421AA8F70A724C2EEA5E0889ACDD398DC016E4093EE6A0FC986EE395341A681EC19A6CF6F794"},
    {"102A2284532F059FD8214BD1F29BCED7F03D737FD7F6811E", "B520CDF777182C779D3658DF397A861E0119E198D9FAE3C937719958CFB3A970B0BDB138EBA47A1E6BDDEBE26D5AE38A91B79BD160297D8DC05EA936BBC570B170B9C79E5057F7AD"},
    {"176D57619B42BE5D37FDDECDC419631D95C0A90A2C54C0E3", "2437ABC498606416F4E9C5AF538D48933881CB851769D768F8A3D499FB0B34DA45E3CEFA041EB273F280E2E64C62F28915DE120C3D45555E54644852FD63E7E8DE5E9FBC29C8B040"},
}};

std::array<std::byte, 24> parseTrailer(std::string_view text) {
    std::array<std::byte, 24> bytes{};
    auto digit = [](char value) { return value <= '9' ? value - '0' : value - 'A' + 10; };
    for (std::size_t i = 0; i < bytes.size(); i++)
        bytes[i] = static_cast<std::byte>(digit(text[i * 2]) * 16 + digit(text[i * 2 + 1]));
    return bytes;
}

bool decryptProSpiVector(const ProSpiVector& vector) {
    AesKey key{};
    for (std::size_t i = 0; i < key.size(); i++)
        key[i] = static_cast<std::byte>(i * 13 + 1);
    std::array<std::byte, 24> trailer = parseTrailer(vector.trailer);
    std::array<std::byte, 96> data{};
    for (std::size_t i = 0; i < 72; i++)
        data[i] = static_cast<std::byte>(i * 7 + 3);
    std::memcpy(data.data() + 72, trailer.data(), trailer.size());
    if (!decryptAs(EGame::GAME_eBaseballProSpirit, key, data, EncryptedPart::CompressionBlock))
        return false;
    std::string actual = toHex(std::span(data).first(72));
    if (actual != vector.expected)
        std::fprintf(stderr, "  trailer %s: %s\n", std::string(vector.trailer).c_str(), actual.c_str());
    return actual == vector.expected && std::memcmp(data.data() + 72, trailer.data(), trailer.size()) == 0;
}

}

TEST(proSpiDescriptorLookupKeyMatchesCue4Parse) {
    std::array<std::byte, 24> trailer{};
    for (std::size_t i = 0; i < trailer.size(); i++)
        trailer[i] = static_cast<std::byte>(i);
    CHECK(ue5parse::getProSpiDescriptorLookupKey(trailer) == 0x48B7AFA878ACC5ABULL);
}

TEST(proSpiCiphersMatchCue4Parse) {
    for (const ProSpiVector& vector : proSpiVectors)
        CHECK(decryptProSpiVector(vector));
}
