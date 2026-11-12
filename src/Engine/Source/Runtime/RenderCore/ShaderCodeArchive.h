#pragma once

#include <unordered_map>
#include <vector>

#include "CppUE5Parse/VFS/FileSystem.h"
#include "CppUE5Parse/VFS/IoStore.h"
#include "Engine/Source/Runtime/Core/Misc/SecureHash.h"
#include "Engine/Source/Runtime/RenderCore/Shader.h"

namespace ue5parse {

class FileProvider;

}

namespace ue5parse::runtime {

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
    uint64 GetShaderUncompressedSize(int32 ShaderIndex) const;

    TArray<FSHAHash> ShaderMapHashes;
    TArray<FSHAHash> ShaderHashes;
    TArray<FIoChunkId> ShaderGroupIoHashes;
    TArray<FIoStoreShaderMapEntry> ShaderMapEntries;
    TArray<FIoStoreShaderCodeEntry> ShaderEntries;
    TArray<FIoStoreShaderGroupEntry> ShaderGroupEntries;
    TArray<uint32> ShaderIndices;
};

class FShaderCodeLibrary {
public:
    explicit FShaderCodeLibrary(const FileProvider& InProvider) : Provider(InProvider) {}

    Result<void> OpenLibraries();

    bool ContainsShaderMap(const FSHAHash& ShaderMapHash) const { return ShaderMaps.contains(ShaderMapHash); }
    Result<std::vector<std::byte>> ReadShaderCode(const FSHAHash& ShaderMapHash, int32 ResourceIndex) const;
    Result<FSHAHash> GetShaderHash(const FSHAHash& ShaderMapHash, int32 ResourceIndex) const;

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

    struct FShaderLocation {
        const FLibrary* Library = nullptr;
        uint32 ShaderIndex = 0u;
    };

    Result<void> OpenLibrary(const FileEntry& File);
    Result<FShaderLocation> FindShader(const FSHAHash& ShaderMapHash, int32 ResourceIndex) const;
    Result<std::vector<std::byte>> ReadPakShader(const FLibrary& Library, uint32 ShaderIndex) const;
    Result<std::vector<std::byte>> ReadIoStoreShader(const FLibrary& Library, uint32 ShaderIndex) const;
    //! Input must have the codec's input slack after its end.
    Result<void> Decompress(std::span<const std::byte> Input, std::vector<std::byte>& Output, std::size_t UncompressedSize) const;

    const FileProvider& Provider;
    std::vector<FLibrary> Libraries;
    std::unordered_map<FSHAHash, FShaderMapLocation, FSHAHashHasher> ShaderMaps;
};

}
