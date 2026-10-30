#include <algorithm>
#include <random>
#include <string>

#include "PakWriter.h"
#include "Test.h"
#include "ue5parse/vfs/FileSystem.h"

using namespace ue5parse;

namespace {

using Sources = std::vector<std::unique_ptr<FileSource>>;

// Sizes around the block size, a file of one block, an empty file, a non-encoded entry and a delete record.
std::vector<test::PakFileSpec> makeFiles(std::uint32_t seed) {
    std::vector<test::PakFileSpec> files;
    files.push_back({"Maps/Ascent/Ascent.umap", test::makeData(20000, seed, true)});
    files.push_back({"Maps/Bind.umap", test::makeData(5000, seed + 1, false)});
    files.push_back({"Config/DefaultGame.ini", test::makeData(700, seed + 2, true)});
    files.push_back({"Empty.txt", test::makeData(0, seed + 3, false)});
    files.push_back({"Characters/Jett/Jett.uasset", test::makeData(12289, seed + 4, true), false});
    files.push_back({"Removed.uasset", {}, true, true});
    return files;
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

std::size_t countLive(const std::vector<test::PakFileSpec>& specs) {
    return static_cast<std::size_t>(std::count_if(specs.begin(), specs.end(), [](const auto& f) { return !f.deleted; }));
}

// Every file reads back exactly, random ranges equal the same slices, and the path writes back as it was.
void checkContents(const FileSystem& files, const std::vector<test::PakFileSpec>& specs) {
    std::mt19937 random(42);
    CHECK(files.getFiles().size() == countLive(specs));
    for (const test::PakFileSpec& spec : specs) {
        std::string path = "Game/Content/" + spec.path;
        std::optional<FileEntry> file = files.findFile(path);
        if (spec.deleted) {
            CHECK(!file.has_value());
            continue;
        }
        if (!CHECK(file.has_value())) continue;
        char buffer[256];
        CHECK(files.writePath(*file, buffer) == path);
        CHECK(readWhole(files, *file) == spec.data);
        for (int i = 0; i < 50 && !spec.data.empty(); i++) {
            std::size_t offset = random() % spec.data.size();
            std::size_t size = random() % (spec.data.size() - offset + 1);
            test::Bytes range(size);
            CHECK(files.readFile(*file, offset, range).has_value());
            CHECK(std::equal(range.begin(), range.end(), spec.data.begin() + offset));
        }
        test::Bytes past(spec.data.size() + 1);
        CHECK(files.readFile(*file, 0, past).error().code == ErrorCode::ReadPastEnd);
    }
}

}

TEST(mountsAndReadsCompressedPak) {
    std::vector<test::PakFileSpec> specs = makeFiles(1);
    Sources sources;
    test::addPak({}, specs, sources);
    Result<FileSystem> files = mount(std::move(sources));
    REQUIRE(files.has_value());
    CHECK(files->getProblems().empty());
    REQUIRE(files->getArchiveCount() == 1);
    const MountedArchive& archive = files->getArchive(0);
    CHECK(archive.kind == ArchiveKind::Pak);
    CHECK(archive.tree.mountPoint == "Game/Content/");
    REQUIRE(archive.pak != nullptr);
    CHECK(archive.pak->getInfo().version == EPakFileVersion::Utf8PakDirectory);
    CHECK(archive.pak->getInfo().getMethodName(1) == "Zlib");
    checkContents(*files, specs);
}

TEST(readsPakVersionsAndBlockSizes) {
    std::vector<test::PakFileSpec> specs = makeFiles(2);
    // A UTF-8 name, which version 12 stores as FUtf8String. Older versions store names as FString, which the test
    // writer writes as ANSI only, so they get an ASCII name.
    specs.push_back({"Maps/Caf\xc3\xa9.umap", test::makeData(300, 9, true)});
    for (std::int32_t version : {10, 11, 12}) {
        for (bool compress : {true, false}) {
            // 3000 is not a multiple of 2048, so the encoded entries store the block size as a full uint32.
            for (std::uint32_t blockSize : {4096u, 3000u}) {
                test::PakOptions options;
                options.version = version;
                options.compress = compress;
                options.blockSize = blockSize;
                std::vector<test::PakFileSpec> written = specs;
                if (version < 12) written.back().path = "Maps/Cafe.umap";
                Sources sources;
                test::addPak(options, written, sources);
                Result<FileSystem> files = mount(std::move(sources));
                REQUIRE(files.has_value());
                CHECK(files->getProblems().empty());
                checkContents(*files, written);
            }
        }
    }
}

TEST(encryptedPakNeedsItsKey) {
    std::vector<test::PakFileSpec> specs = makeFiles(3);
    for (bool compress : {true, false}) {
        test::PakOptions options;
        options.compress = compress;
        options.key = testKey(1);
        options.keyGuid = FGuid{1, 2, 3, 4};
        test::Bytes bytes = test::writePak(options, specs);

        Sources withKey;
        withKey.push_back(std::make_unique<test::MemoryFile>(options.name, bytes));
        Result<FileSystem> files = mount(std::move(withKey), makeDecryptor(testKey(1), options.keyGuid));
        REQUIRE(files.has_value());
        CHECK(files->getProblems().empty());
        checkContents(*files, specs);

        Sources locked;
        locked.push_back(std::make_unique<test::MemoryFile>(options.name, bytes));
        files = mount(std::move(locked), makeDecryptor(testKey(1)));
        REQUIRE(files.has_value());
        REQUIRE(files->getProblems().size() == 1);
        CHECK(files->getProblems()[0].error.code == ErrorCode::MissingKey);
        CHECK(files->getProblems()[0].name == "pakchunk1-Test.pak");

        Sources wrongKey;
        wrongKey.push_back(std::make_unique<test::MemoryFile>(options.name, bytes));
        files = mount(std::move(wrongKey), makeDecryptor(testKey(2), options.keyGuid));
        REQUIRE(files.has_value());
        REQUIRE(files->getProblems().size() == 1);
        CHECK(files->getProblems()[0].error.code == ErrorCode::BadAesKey);
    }
}

TEST(readsLargePakEntriesInParallelRuns) {
    std::vector<test::PakFileSpec> specs;
    specs.push_back({"Large/Compressible.uasset", test::makeData(5u << 20, 1, true)});
    specs.push_back({"Large/Random.ubulk", test::makeData(3u << 20, 2, false)});
    for (bool encrypted : {false, true})
        for (bool mapped : {true, false}) {
            test::PakOptions options;
            options.blockSize = 65536;
            if (encrypted) options.key = testKey(3);
            Sources sources;
            test::addPak(options, specs, sources, mapped);
            Result<FileSystem> files = mount(std::move(sources), encrypted ? makeDecryptor(testKey(3)) : nullptr);
            REQUIRE(files.has_value());
            CHECK(files->getProblems().empty());
            checkContents(*files, specs);
        }
}

TEST(pakPatchWinsOverIoStoreBase) {
    std::vector<test::Chunk> base;
    base.push_back({FIoChunkId::create(100, 0, EIoChunkType::ExportBundleData), test::makeData(9000, 1, true),
                    "Maps/Bind.umap"});
    base.push_back({FIoChunkId::create(101, 0, EIoChunkType::ExportBundleData), test::makeData(100, 2, true),
                    "Maps/Ascent.umap"});
    std::vector<test::PakFileSpec> patch;
    patch.push_back({"Maps/Bind.umap", test::makeData(4000, 3, true)});
    test::PakOptions patchOptions;
    patchOptions.name = "pakchunk0_1_P.pak";

    Sources sources;
    test::addPak(patchOptions, patch, sources);
    test::addSources(test::writeContainer({}, base), sources);
    Result<FileSystem> files = mount(std::move(sources));
    REQUIRE(files.has_value());
    CHECK(files->getProblems().empty());
    CHECK(files->getFiles().size() == 2);
    std::optional<FileEntry> bind = files->findFile("Game/Content/Maps/Bind.umap");
    REQUIRE(bind.has_value());
    CHECK(files->getArchive(bind->archive).kind == ArchiveKind::Pak);
    CHECK(readWhole(*files, *bind) == patch[0].data);
    std::optional<FileEntry> ascent = files->findFile("Game/Content/Maps/Ascent.umap");
    REQUIRE(ascent.has_value());
    CHECK(readWhole(*files, *ascent) == base[1].data);
}

TEST(equalPaksResolveTheSameInAnyOrder) {
    std::vector<test::PakFileSpec> first{{"Same.uasset", test::makeData(100, 1, true)}};
    std::vector<test::PakFileSpec> second{{"Same.uasset", test::makeData(100, 2, true)}};
    test::PakOptions a;
    a.name = "pakchunk1-A.pak";
    test::PakOptions b;
    b.name = "pakchunk1-B.pak";
    test::Bytes winner;
    for (int order = 0; order < 2; order++) {
        Sources sources;
        test::addPak(order == 0 ? a : b, order == 0 ? first : second, sources);
        test::addPak(order == 0 ? b : a, order == 0 ? second : first, sources);
        Result<FileSystem> files = mount(std::move(sources));
        REQUIRE(files.has_value());
        std::optional<FileEntry> same = files->findFile("Game/Content/Same.uasset");
        REQUIRE(same.has_value());
        test::Bytes bytes = readWhole(*files, *same);
        if (order == 0) winner = bytes;
        CHECK(bytes == winner);
    }
}

TEST(unsupportedPakVersionsAreProblems) {
    // A version 9 footer has one more byte (bIndexIsFrozen); version 13 is newer than UE 5.8.
    test::Bytes v12 = test::writePak({}, makeFiles(4));
    test::Bytes v9 = v12;
    v9.push_back(std::byte{0});
    test::Bytes v13 = v12;
    std::int32_t version = 13;
    std::memcpy(v13.data() + v13.size() - FPakInfo::serializedSize + 21, &version, 4);

    Sources sources;
    sources.push_back(std::make_unique<test::MemoryFile>("pakchunk9-Test.pak", v9));
    sources.push_back(std::make_unique<test::MemoryFile>("pakchunk13-Test.pak", v13));
    sources.push_back(std::make_unique<test::MemoryFile>("pakchunk2-Test.pak", test::Bytes(10)));
    sources.push_back(std::make_unique<test::MemoryFile>("pakchunk3-Test.pak", test::Bytes(1000)));
    Result<FileSystem> files = mount(std::move(sources));
    REQUIRE(files.has_value());
    REQUIRE(files->getProblems().size() == 4);
    for (const ArchiveProblem& problem : files->getProblems()) {
        bool unsupported = problem.name == "pakchunk9-Test.pak" || problem.name == "pakchunk13-Test.pak";
        CHECK(problem.error.code == (unsupported ? ErrorCode::UnsupportedVersion : ErrorCode::BadMagic));
    }
}

// Corruptions of the indexes and of the whole file give a value or an error, never a crash, and whatever mounts
// reads without one.
TEST(corruptPakNeverCrashes) {
    std::vector<test::PakFileSpec> specs = makeFiles(5);
    test::Bytes pak = test::writePak({}, specs);
    std::mt19937 random(5);
    for (int i = 0; i < 4000; i++) {
        test::Bytes corrupt = pak;
        // Half anywhere, half in the last 600 bytes: the indexes and the footer.
        std::size_t at = i % 2 == 0 ? random() % corrupt.size()
                                    : corrupt.size() - 1 - random() % std::min<std::size_t>(corrupt.size(), 600);
        corrupt[at] ^= static_cast<std::byte>(1 + random() % 255);
        if (i % 7 == 0) corrupt.resize(random() % corrupt.size());
        Sources sources;
        sources.push_back(std::make_unique<test::MemoryFile>("pakchunk1-Test.pak", corrupt));
        Result<FileSystem> files = mount(std::move(sources));
        if (!CHECK(files.has_value())) continue;
        for (const FileEntry& file : files->getFiles()) {
            test::Bytes bytes(std::min<std::uint64_t>(files->getFileSize(file), 1 << 20));
            (void)files->readFile(file, 0, bytes);
        }
    }
}
