#include <brotli/decode.h>
#include <libdeflate.h>
#include <lz4.h>
#include <zstd.h>

#include <cstdint>

#include "ue5parse/compress/Decompressor.h"

// ooz: kraken.cpp. Returns the decoded size, or -1.
int Kraken_Decompress(const std::uint8_t* source, std::size_t sourceSize, std::uint8_t* destination,
                      std::size_t destinationSize);

namespace ue5parse {

namespace {

const std::uint8_t* asBytes(std::span<const std::byte> data) { return reinterpret_cast<const std::uint8_t*>(data.data()); }
std::uint8_t* asBytes(std::span<std::byte> data) { return reinterpret_cast<std::uint8_t*>(data.data()); }

class OodleDecompressor final : public Decompressor {
public:
    Result<void> decompress(std::span<const std::byte> input, std::span<std::byte> output) const noexcept override {
        int size = Kraken_Decompress(asBytes(input), input.size(), asBytes(output), output.size());
        if (size < 0 || static_cast<std::size_t>(size) != output.size()) return fail(ErrorCode::Decompression, "Oodle");
        return {};
    }
    // "The decompressor will write outside of the target buffer." (kraken.cpp)
    std::size_t getInputSlack() const noexcept override { return 64; }
    std::size_t getOutputSlack() const noexcept override { return 64; }
};

struct DeflateFree {
    void operator()(libdeflate_decompressor* decompressor) const { libdeflate_free_decompressor(decompressor); }
};

libdeflate_decompressor* threadDeflate() {
    thread_local std::unique_ptr<libdeflate_decompressor, DeflateFree> decompressor{libdeflate_alloc_decompressor()};
    return decompressor.get();
}

class ZlibDecompressor final : public Decompressor {
public:
    explicit ZlibDecompressor(bool gzip) : gzip(gzip) {}
    Result<void> decompress(std::span<const std::byte> input, std::span<std::byte> output) const noexcept override {
        libdeflate_decompressor* decompressor = threadDeflate();
        libdeflate_result result = gzip
            ? libdeflate_gzip_decompress(decompressor, input.data(), input.size(), output.data(), output.size(), nullptr)
            : libdeflate_zlib_decompress(decompressor, input.data(), input.size(), output.data(), output.size(), nullptr);
        if (result != LIBDEFLATE_SUCCESS) return fail(ErrorCode::Decompression, gzip ? "Gzip" : "Zlib");
        return {};
    }
    std::size_t getInputSlack() const noexcept override { return 0; }
    std::size_t getOutputSlack() const noexcept override { return 0; }

private:
    bool gzip;
};

struct ZstdFree {
    void operator()(ZSTD_DCtx* context) const { ZSTD_freeDCtx(context); }
};

class ZstdDecompressor final : public Decompressor {
public:
    Result<void> decompress(std::span<const std::byte> input, std::span<std::byte> output) const noexcept override {
        thread_local std::unique_ptr<ZSTD_DCtx, ZstdFree> context{ZSTD_createDCtx()};
        std::size_t size = ZSTD_decompressDCtx(context.get(), output.data(), output.size(), input.data(), input.size());
        if (ZSTD_isError(size) || size != output.size()) return fail(ErrorCode::Decompression, "Zstd");
        return {};
    }
    std::size_t getInputSlack() const noexcept override { return 0; }
    std::size_t getOutputSlack() const noexcept override { return 0; }
};

class Lz4Decompressor final : public Decompressor {
public:
    Result<void> decompress(std::span<const std::byte> input, std::span<std::byte> output) const noexcept override {
        int size = LZ4_decompress_safe(reinterpret_cast<const char*>(input.data()), reinterpret_cast<char*>(output.data()),
                                       static_cast<int>(input.size()), static_cast<int>(output.size()));
        if (size < 0 || static_cast<std::size_t>(size) != output.size()) return fail(ErrorCode::Decompression, "LZ4");
        return {};
    }
    std::size_t getInputSlack() const noexcept override { return 0; }
    std::size_t getOutputSlack() const noexcept override { return 0; }
};

class BrotliDecompressor final : public Decompressor {
public:
    Result<void> decompress(std::span<const std::byte> input, std::span<std::byte> output) const noexcept override {
        std::size_t size = output.size();
        BrotliDecoderResult result = BrotliDecoderDecompress(input.size(), asBytes(input), &size, asBytes(output));
        if (result != BROTLI_DECODER_RESULT_SUCCESS || size != output.size()) return fail(ErrorCode::Decompression, "Brotli");
        return {};
    }
    std::size_t getInputSlack() const noexcept override { return 0; }
    std::size_t getOutputSlack() const noexcept override { return 0; }
};

}

Codecs::Codecs() {
    set("Oodle", std::make_unique<OodleDecompressor>());
    set("Zlib", std::make_unique<ZlibDecompressor>(false));
    set("Gzip", std::make_unique<ZlibDecompressor>(true));
    set("Zstd", std::make_unique<ZstdDecompressor>());
    set("LZ4", std::make_unique<Lz4Decompressor>());
    set("Brotli", std::make_unique<BrotliDecompressor>());
}

void Codecs::set(FName method, std::unique_ptr<Decompressor> decompressor) {
    for (std::size_t i = 0; i < methods.size(); i++) {
        if (methods[i] != method) continue;
        decompressors[i] = std::move(decompressor);
        return;
    }
    methods.push_back(method);
    decompressors.push_back(std::move(decompressor));
}

const Decompressor* Codecs::find(FName method) const {
    for (std::size_t i = 0; i < methods.size(); i++)
        if (methods[i] == method) return decompressors[i].get();
    return nullptr;
}

}
