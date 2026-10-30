#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "ue5parse/io/FileSource.h"

namespace ue5parse {

namespace {

// A POSIX file descriptor and, when mmap succeeds, a read-only mapping of the whole file. RAII: both close in the
// destructor.
class LocalFile final : public FileSource {
public:
    LocalFile(int descriptor, std::uint64_t size, std::string name) : descriptor(descriptor), size(size), name(std::move(name)) {
        if (size == 0) return;
        void* address = ::mmap(nullptr, static_cast<std::size_t>(size), PROT_READ, MAP_SHARED, descriptor, 0);
        if (address != MAP_FAILED) mapped = {static_cast<const std::byte*>(address), static_cast<std::size_t>(size)};
    }
    LocalFile(const LocalFile&) = delete;
    LocalFile& operator=(const LocalFile&) = delete;
    ~LocalFile() override {
        if (!mapped.empty()) ::munmap(const_cast<std::byte*>(mapped.data()), mapped.size());
        ::close(descriptor);
    }

    std::string_view getName() const noexcept override { return name; }
    std::uint64_t getSize() const noexcept override { return size; }
    std::span<const std::byte> getMappedBytes() const noexcept override { return mapped; }

    Result<void> read(std::uint64_t offset, std::span<std::byte> buffer) const noexcept override {
        if (offset > size || buffer.size() > size - offset) return fail(ErrorCode::ReadPastEnd, "LocalFile.read", offset);
        if (!mapped.empty()) {
            if (!buffer.empty()) std::memcpy(buffer.data(), mapped.data() + offset, buffer.size());
            return {};
        }
        std::size_t done = 0;
        while (done < buffer.size()) {
            ssize_t count = ::pread(descriptor, buffer.data() + done, buffer.size() - done, static_cast<off_t>(offset + done));
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) return fail(ErrorCode::Io, "LocalFile.read", offset + done);
            done += static_cast<std::size_t>(count);
        }
        return {};
    }

private:
    int descriptor;
    std::uint64_t size;
    std::string name;
    std::span<const std::byte> mapped;
};

}

Result<std::unique_ptr<FileSource>> openLocalFile(const std::filesystem::path& path) {
    int descriptor = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (descriptor < 0) return fail(ErrorCode::Io, "openLocalFile.open");
    struct stat status {};
    if (::fstat(descriptor, &status) != 0) {
        ::close(descriptor);
        return fail(ErrorCode::Io, "openLocalFile.fstat");
    }
    return std::make_unique<LocalFile>(descriptor, static_cast<std::uint64_t>(status.st_size), path.filename().string());
}

}
