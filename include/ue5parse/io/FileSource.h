#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>

#include "ue5parse/core/Result.h"

namespace ue5parse {

// A file that several threads read at once, at any offset. There is no seek state.
//
// The app implements this interface for its own sources (a CDN, a manifest). The rule for every interface that the
// app implements: only pure virtual functions and an inline default destructor, so that it links from this
// -fno-rtti library into an app with RTTI.
class FileSource {
public:
    virtual ~FileSource() = default;
    // The name, for errors and for the archive set: "pakchunk0-WindowsClient.utoc".
    virtual std::string_view getName() const noexcept = 0;
    virtual std::uint64_t getSize() const noexcept = 0;
    // A short read is an error.
    virtual Result<void> read(std::uint64_t offset, std::span<std::byte> buffer) const noexcept = 0;
    // The whole file in memory, when the source has it there (a memory-mapped local file); else empty. Readers
    // then decode unencrypted blocks in place, without a copy (design §10.3).
    virtual std::span<const std::byte> getMappedBytes() const noexcept = 0;
};

// Opens a local file for positional reads, memory-mapped when the system allows it. The mapping and the descriptor
// close with the returned object.
Result<std::unique_ptr<FileSource>> openLocalFile(const std::filesystem::path& path);

}
