// ue5ls <Paks folder> [--key 0x...] [--list] [--find <path>] [--read-all]
//
// Mounts every .utoc/.ucas/.pak in the folder, prints the problems and the counts, then:
//   --list       prints every file path
//   --find       prints the archive, chunk and size of one file
//   --read-all   reads every file in parallel and prints the errors and the throughput

#include <tbb/parallel_for.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string_view>
#include <vector>

#include "ue5parse/vfs/FileSystem.h"

using namespace ue5parse;

namespace {

void printError(std::string_view what, const Error& error) {
    std::printf("%.*s: %s at %s, offset %llu\n", static_cast<int>(what.size()), what.data(), getErrorName(error.code),
                error.where, static_cast<unsigned long long>(error.offset));
}

int readAll(const FileSystem& files) {
    std::span<const FileEntry> entries = files.getFiles();
    std::atomic<std::uint64_t> bytes = 0;
    std::atomic<std::size_t> failures = 0;
    auto start = std::chrono::steady_clock::now();
    tbb::parallel_for(std::size_t(0), entries.size(), [&](std::size_t i) {
        std::vector<std::byte> data(files.getFileSize(entries[i]));
        Result<void> read = files.readFile(entries[i], 0, data);
        if (read) {
            bytes += data.size();
            return;
        }
        if (failures++ < 20) {
            char path[1024];
            printError(files.writePath(entries[i], path), read.error());
        }
    });
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::printf("read %zu files, %.1f MB in %.2f s (%.0f MB/s), %zu failed\n", entries.size(), bytes / 1e6, seconds,
                bytes / 1e6 / seconds, failures.load());
    return failures == 0 ? 0 : 1;
}

}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: ue5ls <Paks folder> [--key 0x...] [--list] [--find <path>] [--read-all]\n");
        return 2;
    }
    const char* key = nullptr;
    const char* find = nullptr;
    bool list = false;
    bool all = false;
    for (int i = 2; i < argc; i++) {
        if (std::strcmp(argv[i], "--key") == 0 && i + 1 < argc) key = argv[++i];
        else if (std::strcmp(argv[i], "--find") == 0 && i + 1 < argc) find = argv[++i];
        else if (std::strcmp(argv[i], "--list") == 0) list = true;
        else if (std::strcmp(argv[i], "--read-all") == 0) all = true;
    }

    std::vector<std::unique_ptr<FileSource>> sources;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(argv[1], error)) {
        if (!entry.is_regular_file()) continue;
        Result<std::unique_ptr<FileSource>> file = openLocalFile(entry.path());
        if (!file) {
            printError(entry.path().filename().string(), file.error());
            continue;
        }
        sources.push_back(std::move(*file));
    }
    if (error) {
        std::fprintf(stderr, "cannot list %s\n", argv[1]);
        return 1;
    }

    std::unique_ptr<Decryptor> decryptor;
    if (key != nullptr) {
        Result<AesKey> parsed = parseAesKey(key);
        if (!parsed) {
            printError("--key", parsed.error());
            return 2;
        }
        KeyRing keys;
        keys.add(FGuid{}, *parsed);
        decryptor = std::make_unique<AesDecryptor>(keys);
    }

    auto start = std::chrono::steady_clock::now();
    Result<ArchiveSet> set = readArchives(std::move(sources), std::move(decryptor), nullptr);
    if (!set) {
        printError("mount", set.error());
        return 1;
    }
    FileSystem files(std::move(*set));
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    for (const ArchiveProblem& problem : files.getProblems()) printError(problem.name, problem.error);
    std::printf("mounted %zu archives, %zu files in %.3f s; %zu problems\n", files.getArchiveCount(),
                files.getFiles().size(), seconds, files.getProblems().size());

    if (list) {
        char path[1024];
        for (const FileEntry& file : files.getFiles()) {
            std::string_view text = files.writePath(file, path);
            std::printf("%.*s\n", static_cast<int>(text.size()), text.data());
        }
    }
    if (find != nullptr) {
        std::optional<FileEntry> file = files.findFile(find);
        if (!file) {
            std::printf("%s: not found\n", find);
            return 1;
        }
        std::string_view archive = files.getArchive(file->archive).name;
        std::printf("%s: %.*s, chunk %u, %llu bytes\n", find, static_cast<int>(archive.size()), archive.data(),
                    file->chunk, static_cast<unsigned long long>(files.getFileSize(*file)));
    }
    if (all) return readAll(files);
    return 0;
}
