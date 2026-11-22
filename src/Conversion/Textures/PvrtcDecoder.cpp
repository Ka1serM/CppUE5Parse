#include "Conversion/Textures/PvrtcDecoder.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace ue5parse::conversion {

namespace {

//! Red, green and blue have 5 bits, alpha 4 bits.
using Color = std::array<int, 4>;

struct Word {
    std::uint32_t modulation = 0;
    std::uint32_t color = 0;
};

//! Blocks are in Morton order with y in the low bit, and the larger dimension's extra bits on top.
std::uint32_t twiddle(std::uint32_t sizeX, std::uint32_t sizeY, std::uint32_t x, std::uint32_t y) {
    const std::uint32_t minimum = sizeX < sizeY ? sizeX : sizeY;
    std::uint32_t twiddled = 0;
    int shift = 0;
    for (std::uint32_t bit = 1; bit < minimum; bit <<= 1, shift++) {
        if ((y & bit) != 0)
            twiddled |= 1u << (shift * 2);
        if ((x & bit) != 0)
            twiddled |= 2u << (shift * 2);
    }
    return twiddled | ((sizeY < sizeX ? x : y) >> shift) << (shift * 2);
}

//! The low half of the color word is color A, the high half color B, each opaque when its top bit is set.
Color getColor(std::uint32_t color, bool high) {
    if (!high) {
        if ((color & 0x8000) != 0)
            return {static_cast<int>((color >> 10) & 31), static_cast<int>((color >> 5) & 31), static_cast<int>((color & 0x1e) | ((color & 0x1e) >> 4)), 15};
        return {static_cast<int>(((color >> 7) & 0x1e) | ((color >> 11) & 1)), static_cast<int>(((color >> 3) & 0x1e) | ((color >> 7) & 1)),
                static_cast<int>(((color & 0xe) << 1) | ((color & 0xe) >> 2)), static_cast<int>((color >> 11) & 0xe)};
    }
    if ((color & 0x80000000) != 0)
        return {static_cast<int>((color >> 26) & 31), static_cast<int>((color >> 21) & 31), static_cast<int>((color >> 16) & 31), 15};
    return {static_cast<int>(((color >> 23) & 0x1e) | ((color >> 27) & 1)), static_cast<int>(((color >> 19) & 0x1e) | ((color >> 23) & 1)),
            static_cast<int>(((color >> 15) & 0x1e) | ((color >> 19) & 1)), static_cast<int>((color >> 27) & 0xe)};
}

int wrap(int value, int size) { return ((value % size) + size) % size; }

class Image {
public:
    Image(const std::byte* source, int blocksX, int blocksY, bool twoBits)
        : blocksX(blocksX), blocksY(blocksY), wordWidth(twoBits ? 8 : 4), twoBits(twoBits), words(static_cast<std::size_t>(blocksX) * blocksY),
          modulations(words.size() * 32), modes(words.size() * 32) {
        for (int y = 0; y < blocksY; y++)
            for (int x = 0; x < blocksX; x++) {
                const std::byte* word = source + static_cast<std::size_t>(twiddle(blocksX, blocksY, x, y)) * 8;
                Word& target = words[static_cast<std::size_t>(y) * blocksX + x];
                std::memcpy(&target.modulation, word, 4);
                std::memcpy(&target.color, word + 4, 4);
                unpackModulation(target, x, y);
            }
    }

    std::array<int, 4> getPixel(int x, int y) const {
        const Color first = interpolate(x, y, false);
        const Color second = interpolate(x, y, true);
        int modulation = getModulation(x, y);
        const bool punchThrough = modulation > 10;
        if (punchThrough)
            modulation -= 10;
        std::array<int, 4> pixel;
        for (int c = 0; c < 4; c++)
            pixel[c] = (first[c] * (8 - modulation) + second[c] * modulation) / 8;
        if (punchThrough)
            pixel[3] = 0;
        return pixel;
    }

private:
    int blocksX;
    int blocksY;
    int wordWidth;
    bool twoBits;
    std::vector<Word> words;
    std::vector<int> modulations;
    std::vector<int> modes;

    const Word& getWord(int x, int y) const { return words[static_cast<std::size_t>(wrap(y, blocksY)) * blocksX + wrap(x, blocksX)]; }

    std::size_t getTexel(int x, int y) const { return static_cast<std::size_t>(wrap(y, blocksY * 4)) * blocksX * wordWidth + wrap(x, blocksX * wordWidth); }

    //! A 2 bpp block with the mode bit stores every other texel and interpolates the rest, vertically and horizontally, horizontally only, or vertically only.
    void unpackModulation(const Word& word, int blockX, int blockY) {
        int mode = static_cast<int>(word.color & 1);
        std::uint32_t bits = word.modulation;
        if (twoBits && mode != 0) {
            if ((bits & 1) != 0) {
                mode = (bits & (1u << 20)) != 0 ? 3 : 2;
                bits = (bits & ~(1u << 20)) | ((bits >> 1) & (1u << 20));
            }
            bits = (bits & ~1u) | ((bits >> 1) & 1u);
        }
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < wordWidth; x++) {
                const std::size_t texel = getTexel(blockX * wordWidth + x, blockY * 4 + y);
                modes[texel] = mode;
                if (!twoBits) {
                    const int value = static_cast<int>(bits & 3);
                    bits >>= 2;
                    constexpr int direct[4] = {0, 3, 5, 8};
                    constexpr int punchThrough[4] = {0, 4, 14, 8};
                    modulations[texel] = mode != 0 ? punchThrough[value] : direct[value];
                } else if (mode == 0) {
                    modulations[texel] = (bits & 1) != 0 ? 3 : 0;
                    bits >>= 1;
                } else if (((x ^ y) & 1) == 0) {
                    modulations[texel] = static_cast<int>(bits & 3);
                    bits >>= 2;
                }
            }
    }

    int getModulation(int x, int y) const {
        const std::size_t texel = getTexel(x, y);
        if (!twoBits)
            return modulations[texel];
        constexpr int weights[4] = {0, 3, 5, 8};
        const auto at = [&](int dx, int dy) { return weights[modulations[getTexel(x + dx, y + dy)]]; };
        if (modes[texel] == 0 || ((x ^ y) & 1) == 0)
            return weights[modulations[texel]];
        if (modes[texel] == 1)
            return (at(0, -1) + at(0, 1) + at(-1, 0) + at(1, 0) + 2) / 4;
        if (modes[texel] == 2)
            return (at(-1, 0) + at(1, 0) + 1) / 2;
        return (at(0, -1) + at(0, 1) + 1) / 2;
    }

    //! Each color is bilinearly upscaled between the centers of the four nearest blocks and expanded to 8 bits.
    Color interpolate(int x, int y, bool high) const {
        const int left = x - wordWidth / 2;
        const int top = y - 2;
        const int blockX = (left < 0 ? left - wordWidth + 1 : left) / wordWidth;
        const int blockY = (top < 0 ? top - 3 : top) / 4;
        const int fractionX = left - blockX * wordWidth;
        const int fractionY = top - blockY * 4;
        const Color p = getColor(getWord(blockX, blockY).color, high);
        const Color q = getColor(getWord(blockX + 1, blockY).color, high);
        const Color r = getColor(getWord(blockX, blockY + 1).color, high);
        const Color s = getColor(getWord(blockX + 1, blockY + 1).color, high);
        Color result;
        for (int c = 0; c < 4; c++) {
            const int upper = p[c] * wordWidth + fractionX * (q[c] - p[c]);
            const int lower = r[c] * wordWidth + fractionX * (s[c] - r[c]);
            const int value = upper * 4 + fractionY * (lower - upper);
            if (twoBits)
                result[c] = c < 3 ? (value >> 7) + (value >> 2) : (value >> 5) + (value >> 1);
            else
                result[c] = c < 3 ? (value >> 6) + (value >> 1) : (value >> 4) + value;
        }
        return result;
    }
};

}

void decodePvrtc(const std::byte* source, int blocksX, int blocksY, bool twoBits, int sizeX, int sizeY, float* rgba) {
    const Image image(source, blocksX, blocksY, twoBits);
    for (int y = 0; y < sizeY; y++)
        for (int x = 0; x < sizeX; x++) {
            const std::array<int, 4> pixel = image.getPixel(x, y);
            for (int c = 0; c < 4; c++)
                rgba[(static_cast<std::size_t>(y) * sizeX + x) * 4 + c] = static_cast<float>(pixel[c]) / 255.0f;
        }
}

}
