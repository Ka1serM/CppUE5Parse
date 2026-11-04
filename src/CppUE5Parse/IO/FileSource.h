#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

class FileSource {
public:
    virtual ~FileSource() = default;
    virtual std::string_view getName() const noexcept = 0;
    virtual std::uint64_t getSize() const noexcept = 0;
    virtual Result<void> read(std::uint64_t offset, std::span<std::byte> buffer) const noexcept = 0;
};

Result<std::unique_ptr<FileSource>> openLocalFile(const std::filesystem::path& path);
Result<std::vector<std::unique_ptr<FileSource>>> openLocalFiles(const std::filesystem::path& folder);

}
