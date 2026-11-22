#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/Landscape/LandscapeTextureStorageProvider.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
namespace rt = ue5parse::runtime;

constexpr std::uint8_t edgeNormalX = 200;
constexpr std::uint8_t edgeNormalY = 100;
//! A height step of 128 is one unit, so a unit grid scale gives the normal (-1, 0, 1) / sqrt(2), stored as 37 and 127.
constexpr std::uint16_t heightStep = 128;
constexpr std::uint8_t rampNormalX = 37;
constexpr std::uint8_t rampNormalY = 127;

struct Mip {
    int width = 0;
    int height = 0;
    std::vector<std::byte> source;
};

std::uint16_t rampHeight(int x) {
    return static_cast<std::uint16_t>(32768 + heightStep * x);
}

Mip encodeHeights(int width, int height, int sourceWidth, int sourceHeight, const std::function<std::uint16_t(int, int)>& heightAt) {
    Mip mip{width, height, {}};
    std::uint16_t last = 32768;
    for (int y = 0; y < sourceHeight; y++)
        for (int x = 0; x < sourceWidth; x++) {
            const std::uint16_t delta = static_cast<std::uint16_t>(heightAt(x, y) - last);
            last = heightAt(x, y);
            mip.source.push_back(static_cast<std::byte>(delta >> 8));
            mip.source.push_back(static_cast<std::byte>(delta & 0xff));
        }
    const int edgeCount = (width + height) * 2 - 4;
    mip.source.push_back(static_cast<std::byte>(edgeNormalX - 128));
    mip.source.push_back(static_cast<std::byte>(edgeNormalY - 128));
    for (int index = 1; index < edgeCount; index++) {
        mip.source.push_back(std::byte{0});
        mip.source.push_back(std::byte{0});
    }
    return mip;
}

Mip encodeRamp(int width, int height, int sourceWidth, int sourceHeight) {
    return encodeHeights(width, height, sourceWidth, sourceHeight, [](int x, int) { return rampHeight(x); });
}

ue5parse::MountedGame emptyGame(EGame game) {
    ue5parse::MountOptions options;
    options.game = game;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

void withFactory(EGame game, const std::function<void(rt::ULandscapeTextureStorageProviderFactory&)>& check) {
    ue5parse::FileProvider provider(emptyGame(game));
    rt::FRuntime runtime(provider);
    auto* factory = rt::NewObject<rt::ULandscapeTextureStorageProviderFactory>(rt::GetTransientPackage());
    factory->LandscapeGridScale = rt::FVector(1.0, 1.0, 1.0);
    check(*factory);
}

void addMip(rt::ULandscapeTextureStorageProviderFactory& factory, const Mip& mip) {
    factory.Mips.SetNum(factory.Mips.Num() + 1);
    rt::FLandscapeTexture2DMipMap& entry = factory.Mips.Last();
    entry.SizeX = mip.width;
    entry.SizeY = mip.height;
    entry.bCompressed = true;
}

std::uint8_t at(const std::vector<std::byte>& pixels, int width, int x, int y, int channel) {
    return std::to_integer<std::uint8_t>(pixels[static_cast<std::size_t>((y * width + x) * 4 + channel)]);
}

bool isEdge(const Mip& mip, int x, int y) {
    return x == 0 || y == 0 || x == mip.width - 1 || y == mip.height - 1;
}

void checkRamp(const Mip& mip, const std::vector<std::byte>& pixels, const std::function<int(int)>& sourceColumn) {
    for (int y = 0; y < mip.height; y++)
        for (int x = 0; x < mip.width; x++) {
            const std::uint16_t height = rampHeight(sourceColumn(x));
            CHECK(at(pixels, mip.width, x, y, 1) == (height & 0xff));
            CHECK(at(pixels, mip.width, x, y, 2) == (height >> 8));
            CHECK(at(pixels, mip.width, x, y, 0) == (isEdge(mip, x, y) ? edgeNormalX : rampNormalX));
            CHECK(at(pixels, mip.width, x, y, 3) == (isEdge(mip, x, y) ? edgeNormalY : rampNormalY));
        }
}

void checkPlainRamp(EGame game) {
    const Mip mip = encodeRamp(5, 4, 5, 4);
    withFactory(game, [&](rt::ULandscapeTextureStorageProviderFactory& factory) {
        addMip(factory, mip);
        std::vector<std::byte> pixels(static_cast<std::size_t>(mip.width * mip.height * 4));
        CHECK(factory.DecompressMip(mip.source, pixels, 0, game).has_value());
        checkRamp(mip, pixels, [](int x) { return x; });
    });
}

//! Raising (2, 2) by one unit gives (2, 2) the normal (0, 0, 1) and (1, 2) the sum 4 (0, 0, 1) + (-1, 0, 1) / sqrt(2) + (-1, 1, 1) / sqrt(3).
void checkBump(EGame game) {
    const Mip mip = encodeHeights(5, 5, 5, 5, [](int x, int y) { return static_cast<std::uint16_t>(x == 2 && y == 2 ? 32768 + heightStep : 32768); });
    withFactory(game, [&](rt::ULandscapeTextureStorageProviderFactory& factory) {
        addMip(factory, mip);
        std::vector<std::byte> pixels(static_cast<std::size_t>(mip.width * mip.height * 4));
        CHECK(factory.DecompressMip(mip.source, pixels, 0, game).has_value());
        CHECK(at(pixels, mip.width, 2, 2, 0) == 127);
        CHECK(at(pixels, mip.width, 2, 2, 3) == 127);
        CHECK(at(pixels, mip.width, 1, 2, 0) == 97);
        CHECK(at(pixels, mip.width, 1, 2, 3) == 140);
        CHECK(at(pixels, mip.width, 3, 2, 0) == 157);
        CHECK(at(pixels, mip.width, 3, 2, 3) == 114);
    });
}

}

TEST(landscapeMipDecodesOriginalBumpNormals) {
    checkBump(EGame::GAME_UE5_3);
}

TEST(landscapeMipDecodesSinglePassBumpNormals) {
    checkBump(EGame::GAME_UE5_6);
}

TEST(landscapeMipDecodesOriginalNormals) {
    checkPlainRamp(EGame::GAME_UE5_3);
}

TEST(landscapeMipDecodesSinglePassNormals) {
    checkPlainRamp(EGame::GAME_UE5_6);
}

TEST(landscapeMipRestoresBoundaryDuplicates) {
    const Mip mip = encodeRamp(7, 7, 6, 6);
    withFactory(EGame::GAME_UE5_8, [&](rt::ULandscapeTextureStorageProviderFactory& factory) {
        addMip(factory, mip);
        factory.BoundaryOffset = 4;
        factory.BoundaryCountX = 1;
        factory.BoundaryCountY = 1;
        std::vector<std::byte> pixels(static_cast<std::size_t>(mip.width * mip.height * 4));
        CHECK(factory.DecompressMip(mip.source, pixels, 0, EGame::GAME_UE5_8).has_value());
        checkRamp(mip, pixels, [](int x) { return x < 4 ? x : x - 1; });
        for (int index = 0; index < mip.width; index++)
            for (int channel = 0; channel < 4; channel++) {
                CHECK(at(pixels, mip.width, 4, index, channel) == at(pixels, mip.width, 3, index, channel));
                CHECK(at(pixels, mip.width, index, 4, channel) == at(pixels, mip.width, index, 3, channel));
            }
    });
}

TEST(landscapeMipRejectsWrongSourceSize) {
    Mip mip = encodeRamp(5, 4, 5, 4);
    mip.source.pop_back();
    for (EGame game : {EGame::GAME_UE5_3, EGame::GAME_UE5_6})
        withFactory(game, [&](rt::ULandscapeTextureStorageProviderFactory& factory) {
            addMip(factory, mip);
            std::vector<std::byte> pixels(static_cast<std::size_t>(mip.width * mip.height * 4));
            CHECK(!factory.DecompressMip(mip.source, pixels, 0, game).has_value());
        });
}
