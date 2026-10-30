#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"

namespace ue5parse {

// Decompresses one block. Several threads call it at once.
class Decompressor {
public:
    virtual ~Decompressor() = default;
    // Fills output exactly. A size mismatch or corrupt input is ErrorCode::Decompression.
    virtual Result<void> decompress(std::span<const std::byte> input, std::span<std::byte> output) const noexcept = 0;
    // Bytes that the codec may read past the input and write past the output. ooz needs 64 for both.
    virtual std::size_t getInputSlack() const noexcept = 0;
    virtual std::size_t getOutputSlack() const noexcept = 0;
};

// Method name -> decompressor. The names are the names in the utoc and pak method tables; the lookup ignores case.
// One table for each mount: there is no global state.
class Codecs {
public:
    // Oodle (ooz), Zlib, Gzip, Zstd, LZ4, Brotli.
    Codecs();
    Codecs(Codecs&&) noexcept = default;
    Codecs& operator=(Codecs&&) noexcept = default;

    // Adds or replaces, as CUE4Parse's UseNativeOodle does.
    void set(FName method, std::unique_ptr<Decompressor> decompressor);
    const Decompressor* find(FName method) const;

private:
    std::vector<FName> methods;
    std::vector<std::unique_ptr<Decompressor>> decompressors;
};

}
