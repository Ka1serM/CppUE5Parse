#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

class Decompressor {
public:
    virtual ~Decompressor() = default;
    virtual Result<void> decompress(std::span<const std::byte> input, std::span<std::byte> output) const noexcept = 0;
    virtual std::size_t getInputSlack() const noexcept = 0;
    virtual std::size_t getOutputSlack() const noexcept = 0;
};

class Codecs {
public:
    Codecs();
    Codecs(Codecs&&) noexcept = default;
    Codecs& operator=(Codecs&&) noexcept = default;

    void set(FName method, std::unique_ptr<Decompressor> decompressor);
    const Decompressor* find(FName method) const;

private:
    std::vector<FName> methods;
    std::vector<std::unique_ptr<Decompressor>> decompressors;
};

}
