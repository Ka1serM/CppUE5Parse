#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#endif

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>

#include "ue5parse/io/FileSource.h"

namespace ue5parse {

namespace {

#ifdef _WIN32
using NativeFile = HANDLE;
const NativeFile invalidFile = INVALID_HANDLE_VALUE;

// FILE_FLAG_OVERLAPPED: reads from several threads run at the same time instead of one after the other.
NativeFile openNative(const std::filesystem::path& path) {
    return ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
}
void closeNative(NativeFile file) { ::CloseHandle(file); }

// The bytes read at offset, at most size; -1 on an error.
std::int64_t readNative(NativeFile file, std::uint64_t offset, std::byte* buffer, std::size_t size) {
    OVERLAPPED request{};
    request.Offset = static_cast<DWORD>(offset);
    request.OffsetHigh = static_cast<DWORD>(offset >> 32);
    request.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (request.hEvent == nullptr) return -1;
    DWORD count = 0;
    const DWORD length = static_cast<DWORD>(std::min<std::size_t>(size, 1u << 30));
    bool done = ::ReadFile(file, buffer, length, nullptr, &request) || ::GetLastError() == ERROR_IO_PENDING;
    done = done && ::GetOverlappedResult(file, &request, &count, TRUE);
    ::CloseHandle(request.hEvent);
    return done ? count : -1;
}
#else
using NativeFile = int;
const NativeFile invalidFile = -1;

NativeFile openNative(const std::filesystem::path& path) { return ::open(path.c_str(), O_RDONLY | O_CLOEXEC); }
void closeNative(NativeFile file) { ::close(file); }

// The bytes read at offset, at most size; -1 on an error.
std::int64_t readNative(NativeFile file, std::uint64_t offset, std::byte* buffer, std::size_t size) {
    for (;;) {
        const ssize_t count = ::pread(file, buffer, size, static_cast<off_t>(offset));
        if (count >= 0 || errno != EINTR) return count;
    }
}
#endif

// A local file, read by position: no mapping and no seek state, so any number of threads read it at once, and the
// browser (WasmFS) reads it like any other system. RAII: the file closes in the destructor.
class LocalFile final : public FileSource {
public:
    LocalFile(NativeFile file, std::uint64_t size, std::string name) : file(file), size(size), name(std::move(name)) {}
    LocalFile(const LocalFile&) = delete;
    LocalFile& operator=(const LocalFile&) = delete;
    ~LocalFile() override { closeNative(file); }

    std::string_view getName() const noexcept override { return name; }
    std::uint64_t getSize() const noexcept override { return size; }

    Result<void> read(std::uint64_t offset, std::span<std::byte> buffer) const noexcept override {
        if (offset > size || buffer.size() > size - offset) return fail(ErrorCode::ReadPastEnd, "LocalFile.read", offset);
        std::size_t done = 0;
        while (done < buffer.size()) {
            const std::int64_t count = readNative(file, offset + done, buffer.data() + done, buffer.size() - done);
            if (count <= 0) return fail(ErrorCode::Io, "LocalFile.read", offset + done);
            done += static_cast<std::size_t>(count);
        }
        return {};
    }

private:
    NativeFile file;
    std::uint64_t size;
    std::string name;
};

}

Result<std::unique_ptr<FileSource>> openLocalFile(const std::filesystem::path& path) {
    std::error_code error;
    const std::uintmax_t size = std::filesystem::file_size(path, error);
    if (error) return fail(ErrorCode::Io, "openLocalFile.size");
    const NativeFile file = openNative(path);
    if (file == invalidFile) return fail(ErrorCode::Io, "openLocalFile.open");
    return std::make_unique<LocalFile>(file, size, path.filename().string());
}

Result<std::vector<std::unique_ptr<FileSource>>> openLocalFiles(const std::filesystem::path& folder) {
    std::vector<std::unique_ptr<FileSource>> files;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folder, error)) {
        if (!entry.is_regular_file()) continue;
        Result<std::unique_ptr<FileSource>> file = openLocalFile(entry.path());
        if (!file) return std::unexpected(file.error());
        files.push_back(std::move(*file));
    }
    if (error) return fail(ErrorCode::Io, "openLocalFiles.list");
    return files;
}

}
