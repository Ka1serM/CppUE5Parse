#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

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
};

// Opens a local file for reads by position. The file closes with the returned object.
Result<std::unique_ptr<FileSource>> openLocalFile(const std::filesystem::path& path);
// Opens every file of a local folder, such as the game's Paks folder, with openLocalFile. A file that does not open is
// an error.
Result<std::vector<std::unique_ptr<FileSource>>> openLocalFiles(const std::filesystem::path& folder);

}
