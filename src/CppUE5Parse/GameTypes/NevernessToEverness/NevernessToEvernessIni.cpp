#include "CppUE5Parse/GameTypes/NevernessToEverness/NevernessToEvernessIni.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "CppUE5Parse/Crypto/Decryptor.h"

namespace ue5parse {

namespace {

//! The global key first, then the Chinese key.
constexpr std::array<std::string_view, 2> nevernessToEvernessKeys = {"UVbP6pjjw5KZhvddie3tfhg1pVkkveY8", "1zh6IOlIohrR88UNPjiLisrkWACUQYuz"};
constexpr std::string_view splitToken = "|SPLIT|";
//! CUE4Parse ends each line with Environment.NewLine, and FModel runs on Windows.
constexpr std::string_view newLine = "\r\n";
constexpr std::string_view byteOrderMark = "\xEF\xBB\xBF";

bool isWhiteSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && isWhiteSpace(text.front()))
        text.remove_prefix(1);
    while (!text.empty() && isWhiteSpace(text.back()))
        text.remove_suffix(1);
    return text;
}

int getBase64Value(char c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    return -1;
}

//! Follows Convert.TryFromBase64String: white space is skipped and up to two padding characters may end the text.
std::optional<std::vector<std::byte>> decodeBase64(std::string_view text) {
    std::string digits;
    for (char c : text)
        if (!isWhiteSpace(c))
            digits.push_back(c);
    if (digits.size() % 4 != 0)
        return std::nullopt;
    std::size_t padding = 0;
    while (padding < 2 && padding < digits.size() && digits[digits.size() - 1 - padding] == '=')
        padding++;
    std::vector<std::byte> out;
    std::uint32_t bits = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < digits.size() - padding; i++) {
        int value = getBase64Value(digits[i]);
        if (value < 0)
            return std::nullopt;
        bits = (bits << 6) | static_cast<std::uint32_t>(value);
        if (++count % 4 == 0)
            for (int shift = 16; shift >= 0; shift -= 8)
                out.push_back(static_cast<std::byte>((bits >> shift) & 0xFF));
    }
    if (padding == 2) {
        out.push_back(static_cast<std::byte>((bits >> 4) & 0xFF));
    } else if (padding == 1) {
        out.push_back(static_cast<std::byte>((bits >> 10) & 0xFF));
        out.push_back(static_cast<std::byte>((bits >> 2) & 0xFF));
    }
    return out;
}

std::optional<std::string> decryptLine(std::span<const std::byte> data, std::string_view keyText) {
    AesKey key{};
    for (std::size_t i = 0; i < key.size(); i++)
        key[i] = static_cast<std::byte>(keyText[i]);
    std::vector<std::byte> plain(data.begin(), data.end());
    decryptAesEcb(key, plain);
    std::size_t padding = static_cast<std::size_t>(plain.back());
    if (padding == 0 || padding > 16)
        return std::nullopt;
    for (std::size_t i = plain.size() - padding; i < plain.size(); i++)
        if (static_cast<std::size_t>(plain[i]) != padding)
            return std::nullopt;
    return std::string(reinterpret_cast<const char*>(plain.data()), plain.size() - padding);
}

void appendLine(std::string& result, std::string_view line) {
    result.append(line);
    result.append(newLine);
}

}

Result<std::vector<std::byte>> decryptNevernessToEvernessIni(std::span<const std::byte> file) {
    std::string_view text{reinterpret_cast<const char*>(file.data()), file.size()};
    while (text.starts_with(byteOrderMark))
        text.remove_prefix(byteOrderMark.size());
    if (trim(text).empty())
        return std::vector<std::byte>(file.begin(), file.end());
    //! CUE4Parse keeps the first key that works for all later lines.
    std::optional<std::size_t> activeKey;
    std::string result;
    for (std::size_t start = 0; start < text.size();) {
        std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos)
            end = text.size();
        std::string_view raw = text.substr(start, end - start);
        start = end + 1;
        if (raw.empty())
            continue;
        std::string_view line = trim(raw);
        std::optional<std::vector<std::byte>> data = decodeBase64(line);
        if (!data || data->empty() || data->size() % 16 != 0) {
            appendLine(result, line);
            continue;
        }
        std::optional<std::string> decrypted;
        if (activeKey) {
            decrypted = decryptLine(*data, nevernessToEvernessKeys[*activeKey]);
        } else {
            for (std::size_t key = 0; key < nevernessToEvernessKeys.size() && !decrypted; key++) {
                decrypted = decryptLine(*data, nevernessToEvernessKeys[key]);
                if (decrypted)
                    activeKey = key;
            }
            if (!decrypted)
                return fail(ErrorCode::BadAesKey, "decryptNevernessToEvernessIni.key");
        }
        if (!decrypted) {
            appendLine(result, line);
            continue;
        }
        std::string_view parts = *decrypted;
        for (std::size_t from = 0; from < parts.size();) {
            std::size_t split = parts.find(splitToken, from);
            if (split == std::string_view::npos)
                split = parts.size();
            if (split > from)
                appendLine(result, parts.substr(from, split - from));
            from = split + splitToken.size();
        }
    }
    auto bytes = std::as_bytes(std::span(result));
    return std::vector<std::byte>(bytes.begin(), bytes.end());
}

}
