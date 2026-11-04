#include "Conversion/Sounds/SoundDecoder.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <memory>
#include <string>

extern "C" {
#include <libvgmstream.h>
}

namespace ue5parse::conversion {

namespace {

//! vgmstream opens the file again by its name and closes each copy through the close callback.
struct MemoryFile {
    libstreamfile_t streamfile;
    std::span<const std::byte> bytes;
    std::string name;
};

libstreamfile_t* openMemoryFile(std::span<const std::byte> bytes, std::string name);

int readMemoryFile(void* userData, std::uint8_t* destination, const std::int64_t offset, const int length) {
    const auto& file = *static_cast<const MemoryFile*>(userData);
    if (offset < 0 || length <= 0 || static_cast<std::uint64_t>(offset) >= file.bytes.size())
        return 0;
    const std::size_t count = std::min(static_cast<std::size_t>(length), file.bytes.size() - static_cast<std::size_t>(offset));
    std::memcpy(destination, file.bytes.data() + offset, count);
    return static_cast<int>(count);
}

std::int64_t memoryFileSize(void* userData) {
    return static_cast<std::int64_t>(static_cast<const MemoryFile*>(userData)->bytes.size());
}

const char* memoryFileName(void* userData) {
    return static_cast<const MemoryFile*>(userData)->name.c_str();
}

libstreamfile_t* reopenMemoryFile(void* userData, const char* fileName) {
    const auto& file = *static_cast<const MemoryFile*>(userData);
    if (file.name != fileName)
        return nullptr;
    return openMemoryFile(file.bytes, file.name);
}

void closeMemoryFile(libstreamfile_t* streamfile) {
    std::unique_ptr<MemoryFile>(static_cast<MemoryFile*>(streamfile->user_data));
}

libstreamfile_t* openMemoryFile(const std::span<const std::byte> bytes, std::string name) {
    auto file = std::make_unique<MemoryFile>(MemoryFile{.streamfile = {}, .bytes = bytes, .name = std::move(name)});
    file->streamfile = {.user_data = file.get(),
                        .read = readMemoryFile,
                        .get_size = memoryFileSize,
                        .get_name = memoryFileName,
                        .open = reopenMemoryFile,
                        .close = closeMemoryFile};
    return &file.release()->streamfile;
}

struct StreamfileClose {
    void operator()(libstreamfile_t* streamfile) const { libstreamfile_close(streamfile); }
};

struct VgmstreamFree {
    void operator()(libvgmstream_t* stream) const { libvgmstream_free(stream); }
};

}

ConversionResult<Sound> decodeSound(const std::span<const std::byte> file, const std::string_view fileName) {
    libvgmstream_config_t config{};
    config.ignore_loop = true;
    config.force_sfmt = LIBVGMSTREAM_SFMT_PCM16;
    std::unique_ptr<libvgmstream_t, VgmstreamFree> stream;
    {
        const std::unique_ptr<libstreamfile_t, StreamfileClose> streamfile(openMemoryFile(file, std::string(fileName)));
        stream.reset(libvgmstream_create(streamfile.get(), 0, &config));
    }
    if (!stream)
        return conversionFailure(std::format("vgmstream does not recognise the sound '{}'.", fileName));

    const libvgmstream_format_t& format = *stream->format;
    Sound sound{.channels = format.channels, .sampleRate = format.sample_rate, .samples = {}};
    sound.samples.reserve(static_cast<std::size_t>(format.play_samples * format.channels));
    while (!stream->decoder->done) {
        if (libvgmstream_render(stream.get()) < 0)
            return conversionFailure(std::format("vgmstream cannot decode the sound '{}' ({}).", fileName, format.codec_name));
        const auto* samples = static_cast<const std::int16_t*>(stream->decoder->buf);
        sound.samples.insert(sound.samples.end(), samples, samples + static_cast<std::ptrdiff_t>(stream->decoder->buf_samples) * format.channels);
    }
    return sound;
}

}
