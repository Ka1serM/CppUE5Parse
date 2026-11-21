#include "Conversion/Textures/EtcDecoder.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace ue5parse::conversion {

namespace {

using Color = std::array<int, 3>;
using EacValues = std::array<int, 16>;

constexpr int modifierTables[8][2] = {{2, 8}, {5, 17}, {9, 29}, {13, 42}, {18, 60}, {24, 80}, {33, 106}, {47, 183}};
constexpr int paintDistances[8] = {3, 6, 11, 16, 23, 32, 41, 64};
constexpr int eacModifiers[16][8] = {
    {-3, -6, -9, -15, 2, 5, 8, 14}, {-3, -7, -10, -13, 2, 6, 9, 12}, {-2, -5, -8, -13, 1, 4, 7, 12}, {-2, -4, -6, -13, 1, 3, 5, 12},
    {-3, -6, -8, -12, 2, 5, 7, 11}, {-3, -7, -9, -11, 2, 6, 8, 10},  {-4, -7, -8, -11, 3, 6, 7, 10}, {-3, -5, -8, -11, 2, 4, 7, 10},
    {-2, -6, -8, -10, 1, 5, 7, 9},  {-2, -5, -8, -10, 1, 4, 7, 9},   {-2, -4, -8, -10, 1, 3, 7, 9},  {-2, -5, -7, -10, 1, 4, 6, 9},
    {-3, -4, -7, -10, 2, 3, 6, 9},  {-1, -2, -3, -10, 0, 1, 2, 9},   {-4, -6, -8, -9, 3, 5, 7, 8},   {-3, -5, -7, -9, 2, 4, 6, 8},
};

std::uint64_t loadBigEndian(const std::byte* source) {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; i++)
        value = value << 8 | std::to_integer<std::uint64_t>(source[i]);
    return value;
}

int bits(std::uint64_t block, int high, int low) {
    return static_cast<int>((block >> low) & ((1ull << (high - low + 1)) - 1));
}

int extend4(int value) { return value * 17; }

int extend5(int value) { return value << 3 | value >> 2; }

int extend6(int value) { return value << 2 | value >> 4; }

int extend7(int value) { return value << 1 | value >> 6; }

int signExtend3(int value) { return value >= 4 ? value - 8 : value; }

int clampByte(int value) { return std::clamp(value, 0, 255); }

Color offset(const Color& color, int distance) {
    return {clampByte(color[0] + distance), clampByte(color[1] + distance), clampByte(color[2] + distance)};
}

void setColor(float* rgba, const Color& color) {
    rgba[0] = static_cast<float>(color[0]) / 255.0f;
    rgba[1] = static_cast<float>(color[1]) / 255.0f;
    rgba[2] = static_cast<float>(color[2]) / 255.0f;
    rgba[3] = 1.0f;
}

//! The pixel indices run down the columns: the high bits of all pixels are in bits 31 to 16, the low bits in bits 15 to 0.
int getPixelIndex(std::uint64_t block, int x, int y) {
    const int bit = x * 4 + y;
    return static_cast<int>((block >> (bit + 16)) & 1) << 1 | static_cast<int>((block >> bit) & 1);
}

void decodeSubblocks(std::uint64_t block, const Color& first, const Color& second, float* rgba) {
    const bool flip = bits(block, 32, 32) != 0;
    const int tables[2] = {bits(block, 39, 37), bits(block, 36, 34)};
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            const int subblock = flip ? y / 2 : x / 2;
            const int index = getPixelIndex(block, x, y);
            const int magnitude = modifierTables[tables[subblock]][index & 1];
            setColor(rgba + (y * 4 + x) * 4, offset(subblock == 0 ? first : second, (index & 2) != 0 ? -magnitude : magnitude));
        }
}

void decodePaintColors(std::uint64_t block, const std::array<Color, 4>& paint, float* rgba) {
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            setColor(rgba + (y * 4 + x) * 4, paint[getPixelIndex(block, x, y)]);
}

void decodeT(std::uint64_t block, float* rgba) {
    const Color first{extend4(bits(block, 60, 59) << 2 | bits(block, 57, 56)), extend4(bits(block, 55, 52)), extend4(bits(block, 51, 48))};
    const Color second{extend4(bits(block, 47, 44)), extend4(bits(block, 43, 40)), extend4(bits(block, 39, 36))};
    const int distance = paintDistances[bits(block, 35, 34) << 1 | bits(block, 32, 32)];
    decodePaintColors(block, {first, offset(second, distance), second, offset(second, -distance)}, rgba);
}

void decodeH(std::uint64_t block, float* rgba) {
    const int red1 = bits(block, 62, 59);
    const int green1 = bits(block, 58, 56) << 1 | bits(block, 52, 52);
    const int blue1 = bits(block, 51, 51) << 3 | bits(block, 49, 47);
    const int red2 = bits(block, 46, 43);
    const int green2 = bits(block, 42, 39);
    const int blue2 = bits(block, 38, 35);
    const int order = (red1 << 8 | green1 << 4 | blue1) >= (red2 << 8 | green2 << 4 | blue2) ? 1 : 0;
    const int distance = paintDistances[bits(block, 34, 34) << 2 | bits(block, 32, 32) << 1 | order];
    const Color first{extend4(red1), extend4(green1), extend4(blue1)};
    const Color second{extend4(red2), extend4(green2), extend4(blue2)};
    decodePaintColors(block, {offset(first, distance), offset(first, -distance), offset(second, distance), offset(second, -distance)}, rgba);
}

void decodePlanar(std::uint64_t block, float* rgba) {
    const Color origin{extend6(bits(block, 62, 57)), extend7(bits(block, 56, 56) << 6 | bits(block, 54, 49)),
                       extend6(bits(block, 48, 48) << 5 | bits(block, 44, 43) << 3 | bits(block, 41, 39))};
    const Color horizontal{extend6(bits(block, 38, 34) << 1 | bits(block, 32, 32)), extend7(bits(block, 31, 25)), extend6(bits(block, 24, 19))};
    const Color vertical{extend6(bits(block, 18, 13)), extend7(bits(block, 12, 6)), extend6(bits(block, 5, 0))};
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            Color color;
            for (int c = 0; c < 3; c++)
                color[c] = clampByte((x * (horizontal[c] - origin[c]) + y * (vertical[c] - origin[c]) + 4 * origin[c] + 2) >> 2);
            setColor(rgba + (y * 4 + x) * 4, color);
        }
}

//! A differential block whose second red, green or blue base leaves 0 to 31 is a T, H or planar block.
void decodeColorBlock(const std::byte* source, float* rgba) {
    const std::uint64_t block = loadBigEndian(source);
    if (bits(block, 33, 33) == 0) {
        const Color first{extend4(bits(block, 63, 60)), extend4(bits(block, 55, 52)), extend4(bits(block, 47, 44))};
        const Color second{extend4(bits(block, 59, 56)), extend4(bits(block, 51, 48)), extend4(bits(block, 43, 40))};
        decodeSubblocks(block, first, second, rgba);
        return;
    }
    const int red = bits(block, 63, 59);
    const int green = bits(block, 55, 51);
    const int blue = bits(block, 47, 43);
    const int red2 = red + signExtend3(bits(block, 58, 56));
    const int green2 = green + signExtend3(bits(block, 50, 48));
    const int blue2 = blue + signExtend3(bits(block, 42, 40));
    if (red2 < 0 || red2 > 31)
        return decodeT(block, rgba);
    if (green2 < 0 || green2 > 31)
        return decodeH(block, rgba);
    if (blue2 < 0 || blue2 > 31)
        return decodePlanar(block, rgba);
    decodeSubblocks(block, {extend5(red), extend5(green), extend5(blue)}, {extend5(red2), extend5(green2), extend5(blue2)}, rgba);
}

//! An 11-bit block with multiplier 0 applies the modifiers unscaled.
EacValues decodeEacValues(const std::byte* source, bool elevenBits) {
    const std::uint64_t block = loadBigEndian(source);
    const int base = bits(block, 63, 56);
    const int multiplier = bits(block, 55, 52);
    const int* table = eacModifiers[bits(block, 51, 48)];
    EacValues values;
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 4; y++) {
            const int low = 45 - (x * 4 + y) * 3;
            const int modifier = table[bits(block, low + 2, low)];
            if (!elevenBits)
                values[y * 4 + x] = clampByte(base + modifier * multiplier);
            else
                values[y * 4 + x] = std::clamp(base * 8 + 4 + (multiplier != 0 ? modifier * multiplier * 8 : modifier), 0, 2047);
        }
    return values;
}

float unorm11(int value) { return static_cast<float>(value) / 2047.0f; }

}

void decodeEtc2Rgb(const std::byte* source, float* rgba) { decodeColorBlock(source, rgba); }

void decodeEtc2Rgba(const std::byte* source, float* rgba) {
    decodeColorBlock(source + 8, rgba);
    const EacValues alpha = decodeEacValues(source, false);
    for (int i = 0; i < 16; i++)
        rgba[i * 4 + 3] = static_cast<float>(alpha[i]) / 255.0f;
}

void decodeEacR11(const std::byte* source, float* rgba) {
    const EacValues red = decodeEacValues(source, true);
    for (int i = 0; i < 16; i++) {
        const float value = unorm11(red[i]);
        rgba[i * 4] = value;
        rgba[i * 4 + 1] = value;
        rgba[i * 4 + 2] = value;
        rgba[i * 4 + 3] = 1.0f;
    }
}

void decodeEacRg11(const std::byte* source, float* rgba) {
    const EacValues red = decodeEacValues(source, true);
    const EacValues green = decodeEacValues(source + 8, true);
    for (int i = 0; i < 16; i++) {
        rgba[i * 4] = unorm11(red[i]);
        rgba[i * 4 + 1] = unorm11(green[i]);
        rgba[i * 4 + 2] = 0.0f;
        rgba[i * 4 + 3] = 1.0f;
    }
}

}
