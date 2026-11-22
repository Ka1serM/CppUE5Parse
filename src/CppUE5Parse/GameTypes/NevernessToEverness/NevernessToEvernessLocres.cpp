#include "CppUE5Parse/GameTypes/NevernessToEverness/NevernessToEvernessLocres.h"

#include <vector>

#include "CppUE5Parse/Crypto/Decryptor.h"

namespace ue5parse {

namespace {

int decodeDigit(char digit) {
    if (digit >= 'A' && digit <= 'Z')
        return digit - 'A';
    if (digit >= 'a' && digit <= 'z')
        return digit - 'a' + 26;
    if (digit >= '0' && digit <= '9')
        return digit - '0' + 52;
    if (digit == '+' || digit == '-')
        return 62;
    if (digit == '/' || digit == '_')
        return 63;
    return -1;
}

}

Result<std::string> decryptNevernessToEvernessLocresString(std::string_view base64) {
    static const Result<AesKey> key = parseAesKey("0x396d4330686f704b4e6a5377694364684e56375974435765754476484c513238");
    std::vector<std::byte> data;
    data.reserve(base64.size() * 3 / 4);
    std::uint32_t bits = 0;
    int bitCount = 0;
    for (char digit : base64) {
        if (digit == '=')
            break;
        const int value = decodeDigit(digit);
        if (value < 0)
            return fail(ErrorCode::InvalidData, "NevernessToEvernessLocres.Base64");
        bits = (bits << 6) | static_cast<std::uint32_t>(value);
        bitCount += 6;
        if (bitCount < 8)
            continue;
        bitCount -= 8;
        data.push_back(static_cast<std::byte>((bits >> bitCount) & 0xFF));
    }
    if (data.size() % 16 != 0)
        return fail(ErrorCode::InvalidData, "NevernessToEvernessLocres.BlockSize");
    decryptAesEcb(*key, data);
    std::string text(reinterpret_cast<const char*>(data.data()), data.size());
    const std::size_t split = text.find("HottaLocresSplit");
    if (split != std::string::npos)
        text.resize(split);
    return text;
}

}
