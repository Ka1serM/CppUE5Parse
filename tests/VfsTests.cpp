#include <algorithm>
#include <random>
#include <string>

#include "IoStoreWriter.h"
#include "Test.h"
#include "ue5parse/vfs/FileSystem.h"

using namespace ue5parse;

namespace {

using Sources = std::vector<std::unique_ptr<FileSource>>;

FIoChunkId packageChunk(std::uint64_t id) { return FIoChunkId::create(id, 0, EIoChunkType::ExportBundleData); }

std::vector<test::Chunk> makeChunks(std::uint32_t seed) {
    std::vector<test::Chunk> chunks;
    chunks.push_back({packageChunk(100), test::makeData(10000, seed, true), "Maps/Ascent.umap"});
    chunks.push_back({packageChunk(101), test::makeData(5000, seed + 1, false), "Maps/Bind.umap"});
    chunks.push_back({packageChunk(102), test::makeData(1, seed + 2, false), "Characters/Jett/Jett.uasset"});
    chunks.push_back({packageChunk(103), test::makeData(0, seed + 3, false), "Empty.uasset"});
    chunks.push_back({FIoChunkId::create(7, 0, EIoChunkType::ContainerHeader), test::makeData(300, seed + 4, true), ""});
    return chunks;
}

AesKey testKey(std::uint8_t first) {
    AesKey key{};
    for (std::size_t i = 0; i < key.size(); i++) key[i] = static_cast<std::byte>(first + i);
    return key;
}

std::unique_ptr<Decryptor> makeDecryptor(const AesKey& key, FGuid guid = {}) {
    KeyRing keys;
    keys.add(guid, key);
    return std::make_unique<AesDecryptor>(keys);
}

Result<FileSystem> mount(Sources sources, std::unique_ptr<Decryptor> decryptor = nullptr) {
    Result<ArchiveSet> set = readArchives(std::move(sources), std::move(decryptor), nullptr);
    if (!set) return std::unexpected(set.error());
    return FileSystem(std::move(*set));
}

test::Bytes readWhole(const FileSystem& files, const FileEntry& file) {
    test::Bytes bytes(files.getFileSize(file));
    if (!files.readFile(file, 0, bytes)) return {};
    return bytes;
}

// Every file reads back exactly, and random ranges equal the same slices of the whole file.
void checkContents(const FileSystem& files, const std::vector<test::Chunk>& chunks) {
    std::mt19937 random(42);
    for (const test::Chunk& chunk : chunks) {
        if (chunk.path.empty()) continue;
        std::optional<FileEntry> file = files.findFile("Game/Content/" + chunk.path);
        if (!CHECK(file.has_value())) continue;
        CHECK(readWhole(files, *file) == chunk.data);
        for (int i = 0; i < 50 && !chunk.data.empty(); i++) {
            std::size_t offset = random() % chunk.data.size();
            std::size_t size = random() % (chunk.data.size() - offset + 1);
            test::Bytes range(size);
            CHECK(files.readFile(*file, offset, range).has_value());
            CHECK(std::equal(range.begin(), range.end(), chunk.data.begin() + offset));
        }
        test::Bytes past(chunk.data.size() + 1);
        CHECK(files.readFile(*file, 0, past).error().code == ErrorCode::ReadPastEnd);
    }
}

}

TEST(hashPathIgnoresCaseAndEmptyComponents) {
    CHECK(hashPath("Game/Content/Maps") == hashPath("/game//content/MAPS/"));
    CHECK(hashPath("Game/Content") != hashPath("GameContent"));
    CHECK(hashPath("") == 0);
}

TEST(mountsAndReadsCompressedContainer) {
    std::vector<test::Chunk> chunks = makeChunks(1);
    Sources sources;
    test::addSources(test::writeContainer({}, chunks), sources);
    Result<FileSystem> files = mount(std::move(sources));
    REQUIRE(files.has_value());
    CHECK(files->getProblems().empty());
    CHECK(files->getArchiveCount() == 1);
    CHECK(files->getFiles().size() == 4);
    CHECK(files->getArchive(0).tree.mountPoint == "Game/Content/");
    checkContents(*files, chunks);

    std::optional<FileEntry> jett = files->findFile("game/content/characters/JETT/jett.uasset");
    REQUIRE(jett.has_value());
    char buffer[256];
    CHECK(files->writePath(*jett, buffer) == "Game/Content/Characters/Jett/Jett.uasset");
    char small[8];
    CHECK(files->writePath(*jett, small).empty());
    CHECK(!files->findFile("Game/Content/Characters/Jett").has_value());

    Result<std::vector<std::byte>> header = files->readChunk(chunks[4].id);
    REQUIRE(header.has_value());
    CHECK(*header == chunks[4].data);
    CHECK(files->readChunk(packageChunk(999)).error().code == ErrorCode::MissingChunk);
}

TEST(mountsEveryTocVersionOfUe5) {
    for (EIoStoreTocVersion version : {EIoStoreTocVersion::PerfectHashWithOverflow, EIoStoreTocVersion::OnDemandMetaData,
                                       EIoStoreTocVersion::ReplaceIoChunkHashWithIoHash}) {
        std::vector<test::Chunk> chunks = makeChunks(9);
        test::ContainerOptions options;
        options.version = version;
        Sources sources;
        test::addSources(test::writeContainer(options, chunks), sources);
        Result<FileSystem> files = mount(std::move(sources));
        REQUIRE(files.has_value());
        CHECK(files->getProblems().empty());
        checkContents(*files, chunks);
    }
}

TEST(mountsUncompressedContainer) {
    std::vector<test::Chunk> chunks = makeChunks(2);
    test::ContainerOptions options;
    options.compress = false;
    Sources sources;
    test::addSources(test::writeContainer(options, chunks), sources);
    Result<FileSystem> files = mount(std::move(sources));
    REQUIRE(files.has_value());
    checkContents(*files, chunks);
}

TEST(readsAcrossPartitions) {
    std::vector<test::Chunk> chunks = makeChunks(3);
    test::ContainerOptions options;
    options.compress = false;
    options.partitionSize = 6000;
    test::Container container = test::writeContainer(options, chunks);
    REQUIRE(container.partitions.size() > 2);
    Sources sources;
    test::addSources(container, sources);
    Result<FileSystem> files = mount(std::move(sources));
    REQUIRE(files.has_value());
    CHECK(files->getProblems().empty());
    checkContents(*files, chunks);
}

TEST(missingPartitionIsProblem) {
    test::ContainerOptions options;
    options.compress = false;
    options.partitionSize = 6000;
    test::Container container = test::writeContainer(options, makeChunks(3));
    container.partitions.pop_back();
    Sources sources;
    test::addSources(container, sources);
    Result<FileSystem> files = mount(std::move(sources));
    REQUIRE(files.has_value());
    REQUIRE(files->getProblems().size() == 1);
    CHECK(files->getProblems()[0].error.code == ErrorCode::MissingFile);
    CHECK(files->getProblems()[0].name == "pakchunk0-Test.utoc");
}

TEST(encryptedContainerNeedsItsKey) {
    std::vector<test::Chunk> chunks = makeChunks(4);
    test::ContainerOptions options;
    options.key = testKey(1);
    options.keyGuid = FGuid{9, 8, 7, 6};
    test::Container container = test::writeContainer(options, chunks);

    Sources withKey;
    test::addSources(container, withKey);
    Result<FileSystem> files = mount(std::move(withKey), makeDecryptor(testKey(1), options.keyGuid));
    REQUIRE(files.has_value());
    CHECK(files->getProblems().empty());
    checkContents(*files, chunks);

    // No key for the guid: the container stays locked, and that is not an error of the mount.
    Sources locked;
    test::addSources(container, locked);
    files = mount(std::move(locked), makeDecryptor(testKey(1)));
    REQUIRE(files.has_value());
    REQUIRE(files->getProblems().size() == 1);
    CHECK(files->getProblems()[0].error.code == ErrorCode::MissingKey);
    CHECK(files->getFiles().empty());

    Sources wrongKey;
    test::addSources(container, wrongKey);
    files = mount(std::move(wrongKey), makeDecryptor(testKey(2), options.keyGuid));
    REQUIRE(files.has_value());
    REQUIRE(files->getProblems().size() == 1);
    CHECK(files->getProblems()[0].error.code == ErrorCode::BadAesKey);
}

// Ranges longer than one 2 MB run decode in parallel runs; with and without encryption.
TEST(readsLargeChunksInParallelRuns) {
    std::vector<test::Chunk> chunks;
    chunks.push_back({packageChunk(200), test::makeData(5u << 20, 1, true), "Large/Compressible.uasset"});
    chunks.push_back({packageChunk(201), test::makeData(3u << 20, 2, false), "Large/Random.ubulk"});
    for (bool encrypted : {false, true}) {
        test::ContainerOptions options;
        options.blockSize = 65536;
        if (encrypted) options.key = testKey(3);
        Sources sources;
        test::addSources(test::writeContainer(options, chunks), sources);
        Result<FileSystem> files = mount(std::move(sources), encrypted ? makeDecryptor(testKey(3)) : nullptr);
        REQUIRE(files.has_value());
        CHECK(files->getProblems().empty());
        checkContents(*files, chunks);
    }
}

TEST(patchContainerWinsInAnySourceOrder) {
    std::vector<test::Chunk> base = makeChunks(5);
    std::vector<test::Chunk> patch;
    patch.push_back({packageChunk(101), test::makeData(7000, 77, true), "Maps/Bind.umap"});
    patch.push_back({packageChunk(200), test::makeData(100, 78, true), "Maps/New.umap"});
    test::ContainerOptions baseOptions;
    test::ContainerOptions patchOptions;
    patchOptions.name = "pakchunk0_2_P";
    patchOptions.containerId = 2;
    test::Container baseContainer = test::writeContainer(baseOptions, base);
    test::Container patchContainer = test::writeContainer(patchOptions, patch);

    for (int order = 0; order < 2; order++) {
        Sources sources;
        test::addSources(order == 0 ? baseContainer : patchContainer, sources);
        test::addSources(order == 0 ? patchContainer : baseContainer, sources);
        std::mt19937 random(order);
        std::shuffle(sources.begin(), sources.end(), random);
        Result<FileSystem> files = mount(std::move(sources));
        REQUIRE(files.has_value());
        CHECK(files->getProblems().empty());
        CHECK(files->getFiles().size() == 5);

        std::optional<FileEntry> bind = files->findFile("Game/Content/Maps/Bind.umap");
        REQUIRE(bind.has_value());
        CHECK(readWhole(*files, *bind) == patch[0].data);
        Result<std::vector<std::byte>> chunk = files->readChunk(packageChunk(101));
        CHECK(chunk.has_value() && *chunk == patch[0].data);
        std::optional<FileEntry> ascent = files->findFile("Game/Content/Maps/Ascent.umap");
        REQUIRE(ascent.has_value());
        CHECK(readWhole(*files, *ascent) == base[0].data);
        CHECK(files->getArchive(bind->archive).priority == 3 + 100 * 3);
    }
}

TEST(otherFilesAreIgnoredAndBadPakIsReported) {
    Sources sources;
    test::addSources(test::writeContainer({}, makeChunks(6)), sources);
    sources.push_back(std::make_unique<test::MemoryFile>("pakchunk0-Test.sig", test::Bytes(10)));
    sources.push_back(std::make_unique<test::MemoryFile>("pakchunk1-Test.pak", test::Bytes(10)));
    Result<FileSystem> files = mount(std::move(sources));
    REQUIRE(files.has_value());
    REQUIRE(files->getProblems().size() == 1);
    CHECK(files->getProblems()[0].name == "pakchunk1-Test.pak");
    CHECK(files->getProblems()[0].error.code == ErrorCode::BadMagic);
    CHECK(files->getFiles().size() == 4);
}

TEST(cancelledMountFails) {
    Sources sources;
    test::addSources(test::writeContainer({}, makeChunks(7)), sources);
    std::stop_source stop;
    stop.request_stop();
    Result<ArchiveSet> set = readArchives(std::move(sources), nullptr, nullptr, stop.get_token());
    CHECK(!set.has_value() && set.error().code == ErrorCode::Cancelled);
}

// Every truncation and many single-byte corruptions of a TOC give a value or an error, never a crash.
TEST(corruptTocNeverCrashes) {
    test::Container container = test::writeContainer({}, makeChunks(8));
    for (std::size_t size = 0; size < container.toc.size(); size++) {
        test::Container truncated = container;
        truncated.toc.resize(size);
        Sources sources;
        test::addSources(truncated, sources);
        Result<FileSystem> files = mount(std::move(sources));
        CHECK(files.has_value() && files->getProblems().size() == 1);
    }
    std::mt19937 random(8);
    for (int i = 0; i < 2000; i++) {
        test::Container corrupt = container;
        corrupt.toc[random() % corrupt.toc.size()] ^= static_cast<std::byte>(1 + random() % 255);
        Sources sources;
        test::addSources(corrupt, sources);
        Result<FileSystem> files = mount(std::move(sources));
        if (!CHECK(files.has_value())) continue;
        // Whatever mounted must read without a crash.
        for (const FileEntry& file : files->getFiles()) {
            test::Bytes bytes(std::min<std::uint64_t>(files->getFileSize(file), 1 << 20));
            (void)files->readFile(file, 0, bytes);
        }
    }
}
