#pragma once

#include <unordered_map>
#include <vector>

#include "ue5parse/runtime/Core/Misc/SecureHash.h"
#include "ue5parse/runtime/RenderCore/Shader.h"
#include "ue5parse/vfs/FileSystem.h"
#include "ue5parse/vfs/IoStore.h"

namespace ue5parse {
class FileProvider;
}

namespace ue5parse::runtime {

// UE's ShaderCodeArchive.h and ShaderCodeLibrary.cpp: the shader libraries (.ushaderbytecode) that hold the code of
// the shader maps that a cook shares (UE 5.3). A library file starts with its format version: 2 for a library in a pak
// (FSerializedShaderArchive, the code follows the header), 1 for a library in IoStore (FIoStoreShaderCodeArchiveHeader,
// the code is in compressed groups of shaders, one chunk each). The code is Oodle compressed.

struct FShaderMapEntry {
    uint32 ShaderIndicesOffset = 0u;
    uint32 NumShaders = 0u;
    uint32 FirstPreloadIndex = 0u;
    uint32 NumPreloadEntries = 0u;
};

struct FShaderCodeEntry {
    uint64 Offset = 0u;
    uint32 Size = 0u;
    uint32 UncompressedSize = 0u;
    uint8 Frequency = 0u;
};

struct FFileCachePreloadEntry {
    int64 Offset = 0;
    int64 Size = 0;
};

class FSerializedShaderArchive {
public:
    void Serialize(FArchive& Ar);

    TArray<FSHAHash> ShaderMapHashes;
    TArray<FSHAHash> ShaderHashes;
    TArray<FShaderMapEntry> ShaderMapEntries;
    TArray<FShaderCodeEntry> ShaderEntries;
    TArray<FFileCachePreloadEntry> PreloadEntries;
    TArray<uint32> ShaderIndices;
};

struct FIoStoreShaderMapEntry {
    uint32 ShaderIndicesOffset = 0u;
    uint32 NumShaders = 0u;
};

// The frequency in 4 bits, the group in 30 bits, the offset in the uncompressed group in 30 bits.
struct FIoStoreShaderCodeEntry {
    uint64 Packed = 0u;

    EShaderFrequency GetFrequency() const { return static_cast<EShaderFrequency>(Packed & 0xfu); }
    uint32 GetShaderGroupIndex() const { return static_cast<uint32>((Packed >> 4) & 0x3fffffffu); }
    uint32 GetUncompressedOffsetInGroup() const { return static_cast<uint32>(Packed >> 34); }
};

struct FIoStoreShaderGroupEntry {
    uint32 ShaderIndicesOffset = 0u;
    uint32 NumShaders = 0u;
    uint32 UncompressedSize = 0u;
    uint32 CompressedSize = 0u;

    bool IsGroupCompressed() const { return CompressedSize != UncompressedSize; }
};

struct FIoStoreShaderCodeArchiveHeader {
    void Serialize(FArchive& Ar);
    // The size of a shader: up to the next shader of its group, or to the end of the group for the last one. 0 when
    // the shader is not in its own group (a corrupt library).
    uint64 GetShaderUncompressedSize(int32 ShaderIndex) const;

    TArray<FSHAHash> ShaderMapHashes;
    TArray<FSHAHash> ShaderHashes;
    TArray<FIoChunkId> ShaderGroupIoHashes;
    TArray<FIoStoreShaderMapEntry> ShaderMapEntries;
    TArray<FIoStoreShaderCodeEntry> ShaderEntries;
    TArray<FIoStoreShaderGroupEntry> ShaderGroupEntries;
    TArray<uint32> ShaderIndices;
};

// The shader libraries of a mounted game, by shader map hash. Thread-safe after OpenLibraries.
class FShaderCodeLibrary {
public:
    explicit FShaderCodeLibrary(const FileProvider& InProvider) : Provider(InProvider) {}

    // Reads the header of every .ushaderbytecode file. A shader map in more than one library is taken from the first.
    Result<void> OpenLibraries();

    bool ContainsShaderMap(const FSHAHash& ShaderMapHash) const { return ShaderMaps.contains(ShaderMapHash); }
    // The uncompressed code of a shader of a shader map: FShader::ResourceIndex selects it. The code is the platform's
    // shader format with the RHI's header.
    Result<std::vector<std::byte>> ReadShaderCode(const FSHAHash& ShaderMapHash, int32 ResourceIndex) const;

private:
    struct FLibrary {
        FileEntry File;
        uint32 Version = 0u;
        int64 LibraryCodeOffset = 0;
        FSerializedShaderArchive PakArchive;
        FIoStoreShaderCodeArchiveHeader IoStoreHeader;
    };
    struct FShaderMapLocation {
        uint32 Library = 0u;
        uint32 ShaderMap = 0u;
    };

    Result<void> OpenLibrary(const FileEntry& File);
    Result<std::vector<std::byte>> ReadPakShader(const FLibrary& Library, uint32 ShaderIndex) const;
    Result<std::vector<std::byte>> ReadIoStoreShader(const FLibrary& Library, uint32 ShaderIndex) const;
    // Fills Output from Input with Oodle; Input must have the codec's input slack after its end.
    Result<void> Decompress(std::span<const std::byte> Input, std::vector<std::byte>& Output, std::size_t UncompressedSize) const;

    const FileProvider& Provider;
    std::vector<FLibrary> Libraries;
    std::unordered_map<FSHAHash, FShaderMapLocation, FSHAHashHasher> ShaderMaps;
};

}
