#include "Engine/Source/Runtime/RenderCore/ShaderCodeArchive.h"

#include <charconv>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "CppUE5Parse/Provider/FileProvider.h"

namespace ue5parse::runtime {

namespace {

//! UE 4.16 to 4.24 compress shader code with Zlib, UE 4.25 to 4.27 with LZ4, UE5 with Oodle.
const Decompressor* FindShaderCodec(const FileProvider& Provider) {
    if (Provider.getGame() >= EGame::GAME_UE5_0)
        return Provider.getFiles().getCodecs().find(FName("Oodle"));
    return Provider.getFiles().getCodecs().find(FName(Provider.getGame() >= EGame::GAME_UE4_25 ? "LZ4" : "Zlib"));
}

//! UE 5.8 raises the version of the pak shader library to 3 with the switch to FShaderHash.
uint32 GetShaderCodeArchiveVersion(EGame Game) {
    return Game >= EGame::GAME_UE5_8 ? 3u : 2u;
}

constexpr uint32 IoStoreShaderCodeArchiveVersion = 1u;
constexpr uint32 LegacyShaderCodeArchiveVersion = 1u;
constexpr uint64 InitialHeaderReadSize = 1u << 20;
constexpr std::string_view Extension = ".ushaderbytecode";

bool IsShaderLibraryName(std::string_view Name) {
    if (Name.size() < Extension.size())
        return false;
    std::string_view Tail = Name.substr(Name.size() - Extension.size());
    for (std::size_t Index = 0; Index < Extension.size(); Index++) {
        char Lower = Tail[Index] >= 'A' && Tail[Index] <= 'Z' ? static_cast<char>(Tail[Index] - 'A' + 'a') : Tail[Index];
        if (Lower != Extension[Index])
            return false;
    }
    return true;
}

//! UE 4.16 and 4.17 store each shader in a file named after its frequency digit and output hash, as in 3_<SHA>.ushaderbytecode.
bool ParseLooseShaderName(std::string_view Name, FSHAHash& Hash) {
    std::size_t HexSize = 2 * sizeof(Hash.Hash);
    if (Name.size() != 2 + HexSize + Extension.size() || Name[1] != '_')
        return false;
    for (std::size_t Index = 0; Index < sizeof(Hash.Hash); Index++) {
        const char* Digits = Name.data() + 2 + 2 * Index;
        auto [End, Error] = std::from_chars(Digits, Digits + 2, Hash.Hash[Index], 16);
        if (Error != std::errc() || End != Digits + 2)
            return false;
    }
    return true;
}

template <class TParse>
Result<void> ReadLibraryHeader(const FileProvider& Provider, const FileEntry& File, TParse&& Parse) {
    const FileSystem& Files = Provider.getFiles();
    uint64 FileSize = Files.getFileSize(File);
    std::vector<std::byte> Bytes;
    for (uint64 ReadSize = std::min(InitialHeaderReadSize, FileSize);; ReadSize = std::min(ReadSize * 2, FileSize)) {
        Bytes.resize(ReadSize);
        Result<void> Read = Files.readFile(File, 0, Bytes);
        if (!Read)
            return Read;
        FMemoryReaderView Ar(Bytes);
        Ar.SetGame(Provider.getGame());
        Parse(Ar);
        if (!Ar.IsError())
            return {};
        if (Ar.GetError().code != ErrorCode::ReadPastEnd || ReadSize == FileSize)
            return std::unexpected(Ar.GetError());
    }
}

}

FArchive& operator<<(FArchive& Ar, FShaderMapEntry& Value) {
    return Ar << Value.ShaderIndicesOffset << Value.NumShaders << Value.FirstPreloadIndex << Value.NumPreloadEntries;
}

FArchive& operator<<(FArchive& Ar, FShaderCodeEntry& Value) {
    if (Ar.GetGame() >= EGame::GAME_UE4_20) {
        Ar << Value.Offset;
    } else {
        uint32 Offset = 0u;
        Ar << Offset;
        Value.Offset = Offset;
    }
    Ar << Value.Size << Value.UncompressedSize;
    if (Ar.GetGame() >= EGame::GAME_UE4_19)
        Ar << Value.Frequency;
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FFileCachePreloadEntry& Value) {
    return Ar << Value.Offset << Value.Size;
}

FArchive& operator<<(FArchive& Ar, FIoStoreShaderMapEntry& Value) {
    return Ar << Value.ShaderIndicesOffset << Value.NumShaders;
}

FArchive& operator<<(FArchive& Ar, FIoStoreShaderCodeEntry& Value) {
    return Ar << Value.Packed;
}

FArchive& operator<<(FArchive& Ar, FIoStoreShaderGroupEntry& Value) {
    return Ar << Value.ShaderIndicesOffset << Value.NumShaders << Value.UncompressedSize << Value.CompressedSize;
}

FArchive& operator<<(FArchive& Ar, FIoChunkId& Value) {
    Ar.Serialize(Value.bytes, sizeof(Value.bytes));
    return Ar;
}

void FSerializedShaderArchive::Serialize(FArchive& Ar) {
    Ar << ShaderMapHashes;
    Ar << ShaderHashes;
    Ar << ShaderMapEntries;
    Ar << ShaderEntries;
    Ar << PreloadEntries;
    Ar << ShaderIndices;
    if (!Ar.IsError() && (ShaderHashes.Num() != ShaderEntries.Num() || ShaderMapHashes.Num() != ShaderMapEntries.Num()))
        Ar.SetError(ErrorCode::InvalidData, "FSerializedShaderArchive.Entries");
}

void FIoStoreShaderCodeArchiveHeader::Serialize(FArchive& Ar) {
    Ar << ShaderMapHashes;
    Ar << ShaderHashes;
    Ar << ShaderGroupIoHashes;
    Ar << ShaderMapEntries;
    Ar << ShaderEntries;
    Ar << ShaderGroupEntries;
    Ar << ShaderIndices;
    if (!Ar.IsError()
        && (ShaderHashes.Num() != ShaderEntries.Num() || ShaderMapHashes.Num() != ShaderMapEntries.Num()
            || ShaderGroupIoHashes.Num() != ShaderGroupEntries.Num()))
        Ar.SetError(ErrorCode::InvalidData, "FIoStoreShaderCodeArchiveHeader.Entries");
}

uint64 FIoStoreShaderCodeArchiveHeader::GetShaderUncompressedSize(int32 ShaderIndex) const {
    const FIoStoreShaderCodeEntry& ThisShaderEntry = ShaderEntries[ShaderIndex];
    const FIoStoreShaderGroupEntry& GroupEntry = ShaderGroupEntries[static_cast<int32>(ThisShaderEntry.GetShaderGroupIndex())];
    uint64 StopBeforeIdxIdx = static_cast<uint64>(GroupEntry.ShaderIndicesOffset) + GroupEntry.NumShaders;
    for (uint64 ShaderIdxIdx = GroupEntry.ShaderIndicesOffset; ShaderIdxIdx < StopBeforeIdxIdx; ++ShaderIdxIdx) {
        if (ShaderIndices[static_cast<int32>(ShaderIdxIdx)] != static_cast<uint32>(ShaderIndex))
            continue;
        if (ShaderIdxIdx < StopBeforeIdxIdx - 1) {
            const FIoStoreShaderCodeEntry& NextShaderEntry = ShaderEntries[static_cast<int32>(ShaderIndices[static_cast<int32>(ShaderIdxIdx + 1)])];
            return NextShaderEntry.GetUncompressedOffsetInGroup() - ThisShaderEntry.GetUncompressedOffsetInGroup();
        }
        return GroupEntry.UncompressedSize - ThisShaderEntry.GetUncompressedOffsetInGroup();
    }
    return 0u;
}

Result<void> FShaderCodeLibrary::OpenLibraries() {
    for (const FileEntry& File : Provider.getFiles().getFiles()) {
        if (!IsShaderLibraryName(Provider.getFiles().getFileName(File)))
            continue;
        Result<void> Opened = Provider.getGame() >= EGame::GAME_UE4_25 ? OpenLibrary(File) : OpenLegacyLibrary(File);
        if (!Opened)
            return Opened;
    }
    return {};
}

Result<void> FShaderCodeLibrary::OpenLibrary(const FileEntry& File) {
    FLibrary Library;
    Result<void> Read = ReadLibraryHeader(Provider, File, [&](FArchive& Ar) {
        Library = FLibrary();
        Library.File = File;
        Ar << Library.Version;
        if (Library.Version == GetShaderCodeArchiveVersion(Provider.getGame())) {
            Library.PakArchive.Serialize(Ar);
            Library.LibraryCodeOffset = Ar.Tell();
        } else if (Library.Version == IoStoreShaderCodeArchiveVersion) {
            Library.IoStoreHeader.Serialize(Ar);
        } else if (!Ar.IsError()) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "FShaderCodeLibrary.Version");
        }
    });
    if (!Read)
        return Read;

    uint32 LibraryIndex = static_cast<uint32>(Libraries.size());
    const TArray<FShaderCodeHash>& ShaderMapHashes =
        Library.Version != IoStoreShaderCodeArchiveVersion ? Library.PakArchive.ShaderMapHashes : Library.IoStoreHeader.ShaderMapHashes;
    ShaderMaps.reserve(ShaderMaps.size() + static_cast<std::size_t>(ShaderMapHashes.Num()));
    for (int32 Index = 0; Index < ShaderMapHashes.Num(); Index++)
        ShaderMaps.try_emplace(ShaderMapHashes[Index], FShaderMapLocation{LibraryIndex, static_cast<uint32>(Index)});
    Libraries.push_back(std::move(Library));
    return {};
}

//! UE 4.18 and 4.19 write no version, 4.20 to 4.24 write version 1, and all map the output hash of each shader to its code.
Result<void> FShaderCodeLibrary::OpenLegacyLibrary(const FileEntry& File) {
    if (Provider.getGame() < EGame::GAME_UE4_18)
        return OpenLooseShader(File);
    std::vector<std::pair<FShaderCodeHash, FShaderCodeEntry>> Entries;
    int64 CodeOffset = 0;
    Result<void> Read = ReadLibraryHeader(Provider, File, [&](FArchive& Ar) {
        Entries.clear();
        uint32 Version = LegacyShaderCodeArchiveVersion;
        if (Ar.GetGame() >= EGame::GAME_UE4_20)
            Ar << Version;
        if (!Ar.IsError() && Version != LegacyShaderCodeArchiveVersion) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "FShaderCodeLibrary.Version");
            return;
        }
        int32 Num = 0;
        Ar << Num;
        if (!Ar.IsError() && Num < 0)
            Ar.SetError(ErrorCode::InvalidData, "FShaderCodeLibrary.Shaders");
        for (int32 Index = 0; Index < Num && !Ar.IsError(); Index++) {
            auto& [Hash, Entry] = Entries.emplace_back();
            Ar << Hash << Entry;
        }
        CodeOffset = Ar.Tell();
    });
    if (!Read)
        return Read;
    LegacyShaders.reserve(LegacyShaders.size() + Entries.size());
    for (const auto& [Hash, Entry] : Entries)
        LegacyShaders.try_emplace(Hash, FStoredShader{File, static_cast<uint64>(CodeOffset) + Entry.Offset, Entry.Size, Entry.UncompressedSize});
    return {};
}

//! A loose shader file holds the uint32 uncompressed size and the TArray<uint8> code.
Result<void> FShaderCodeLibrary::OpenLooseShader(const FileEntry& File) {
    const FileSystem& Files = Provider.getFiles();
    FSHAHash Hash;
    if (!ParseLooseShaderName(Files.getFileName(File), Hash))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.LooseShaderName");
    std::array<std::byte, 2 * sizeof(uint32)> Header{};
    Result<void> Read = Files.readFile(File, 0, Header);
    if (!Read)
        return Read;
    FMemoryReaderView Ar(Header);
    uint32 UncompressedSize = 0u;
    int32 Size = 0;
    Ar << UncompressedSize << Size;
    if (Size < 0 || Header.size() + static_cast<uint64>(Size) > Files.getFileSize(File))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.LooseShaderSize");
    LegacyShaders.try_emplace(Hash, FStoredShader{File, Header.size(), static_cast<uint32>(Size), UncompressedSize});
    return {};
}

Result<std::vector<std::byte>> FShaderCodeLibrary::ReadShaderCode(const FShaderCodeHash& ShaderHash) const {
    auto Found = LegacyShaders.find(ShaderHash);
    if (Found == LegacyShaders.end())
        return fail(ErrorCode::MissingChunk, "FShaderCodeLibrary.Shader");
    return ReadStoredShader(Found->second);
}

Result<std::vector<std::byte>> FShaderCodeLibrary::ReadShaderCode(const FShaderCodeHash& ShaderMapHash, int32 ResourceIndex) const {
    Result<FShaderLocation> Shader = FindShader(ShaderMapHash, ResourceIndex);
    if (!Shader)
        return std::unexpected(Shader.error());
    if (Shader->Library->Version != IoStoreShaderCodeArchiveVersion)
        return ReadPakShader(*Shader->Library, Shader->ShaderIndex);
    return ReadIoStoreShader(*Shader->Library, Shader->ShaderIndex);
}

Result<FShaderCodeHash> FShaderCodeLibrary::GetShaderHash(const FShaderCodeHash& ShaderMapHash, int32 ResourceIndex) const {
    Result<FShaderLocation> Shader = FindShader(ShaderMapHash, ResourceIndex);
    if (!Shader)
        return std::unexpected(Shader.error());
    const FLibrary& Library = *Shader->Library;
    const TArray<FShaderCodeHash>& ShaderHashes = Library.Version != IoStoreShaderCodeArchiveVersion ? Library.PakArchive.ShaderHashes : Library.IoStoreHeader.ShaderHashes;
    if (!ShaderHashes.IsValidIndex(static_cast<int32>(Shader->ShaderIndex)))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderHashes");
    return ShaderHashes[static_cast<int32>(Shader->ShaderIndex)];
}

Result<FShaderCodeLibrary::FShaderLocation> FShaderCodeLibrary::FindShader(const FShaderCodeHash& ShaderMapHash, int32 ResourceIndex) const {
    auto Found = ShaderMaps.find(ShaderMapHash);
    if (Found == ShaderMaps.end())
        return fail(ErrorCode::MissingChunk, "FShaderCodeLibrary.ShaderMap");
    const FLibrary& Library = Libraries[Found->second.Library];
    int32 ShaderMapIndex = static_cast<int32>(Found->second.ShaderMap);
    uint32 ShaderIndicesOffset = 0u;
    uint32 NumShaders = 0u;
    if (Library.Version != IoStoreShaderCodeArchiveVersion) {
        ShaderIndicesOffset = Library.PakArchive.ShaderMapEntries[ShaderMapIndex].ShaderIndicesOffset;
        NumShaders = Library.PakArchive.ShaderMapEntries[ShaderMapIndex].NumShaders;
    } else {
        ShaderIndicesOffset = Library.IoStoreHeader.ShaderMapEntries[ShaderMapIndex].ShaderIndicesOffset;
        NumShaders = Library.IoStoreHeader.ShaderMapEntries[ShaderMapIndex].NumShaders;
    }
    if (ResourceIndex < 0 || static_cast<uint32>(ResourceIndex) >= NumShaders)
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ResourceIndex");
    const TArray<uint32>& ShaderIndices = Library.Version != IoStoreShaderCodeArchiveVersion ? Library.PakArchive.ShaderIndices : Library.IoStoreHeader.ShaderIndices;
    int32 IndexIndex = static_cast<int32>(ShaderIndicesOffset + static_cast<uint32>(ResourceIndex));
    if (!ShaderIndices.IsValidIndex(IndexIndex))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderIndices");
    return FShaderLocation{&Library, ShaderIndices[IndexIndex]};
}

Result<std::vector<std::byte>> FShaderCodeLibrary::ReadPakShader(const FLibrary& Library, uint32 ShaderIndex) const {
    if (!Library.PakArchive.ShaderEntries.IsValidIndex(static_cast<int32>(ShaderIndex)))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderEntries");
    const FShaderCodeEntry& Entry = Library.PakArchive.ShaderEntries[static_cast<int32>(ShaderIndex)];
    return ReadStoredShader(FStoredShader{Library.File, static_cast<uint64>(Library.LibraryCodeOffset) + Entry.Offset, Entry.Size, Entry.UncompressedSize});
}

Result<std::vector<std::byte>> FShaderCodeLibrary::ReadStoredShader(const FStoredShader& Shader) const {
    const Decompressor* Codec = FindShaderCodec(Provider);
    std::size_t Slack = Codec != nullptr ? Codec->getInputSlack() : 0u;
    std::vector<std::byte> Code(Shader.Size + Slack);
    Result<void> Read = Provider.getFiles().readFile(Shader.File, Shader.Offset, std::span(Code).first(Shader.Size));
    if (!Read)
        return std::unexpected(Read.error());
    if (Shader.Size == Shader.UncompressedSize) {
        Code.resize(Shader.Size);
        return Code;
    }
    std::vector<std::byte> Uncompressed;
    Result<void> Decompressed = Decompress(std::span(Code).first(Shader.Size), Uncompressed, Shader.UncompressedSize);
    if (!Decompressed)
        return std::unexpected(Decompressed.error());
    return Uncompressed;
}

Result<std::vector<std::byte>> FShaderCodeLibrary::ReadIoStoreShader(const FLibrary& Library, uint32 ShaderIndex) const {
    const FIoStoreShaderCodeArchiveHeader& Header = Library.IoStoreHeader;
    if (!Header.ShaderEntries.IsValidIndex(static_cast<int32>(ShaderIndex)))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderEntries");
    const FIoStoreShaderCodeEntry& Entry = Header.ShaderEntries[static_cast<int32>(ShaderIndex)];
    int32 GroupIndex = static_cast<int32>(Entry.GetShaderGroupIndex());
    if (!Header.ShaderGroupEntries.IsValidIndex(GroupIndex))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderGroupEntries");
    const FIoStoreShaderGroupEntry& Group = Header.ShaderGroupEntries[GroupIndex];
    uint64 Size = Header.GetShaderUncompressedSize(static_cast<int32>(ShaderIndex));
    uint64 Offset = Entry.GetUncompressedOffsetInGroup();
    if (Size == 0u || Offset + Size > Group.UncompressedSize)
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderSize");

    const FileSystem& Files = Provider.getFiles();
    std::optional<ChunkLocation> Chunk = Files.findChunk(Header.ShaderGroupIoHashes[GroupIndex]);
    if (!Chunk)
        return fail(ErrorCode::MissingChunk, "FShaderCodeLibrary.ShaderGroup");
    if (Files.getChunkSize(*Chunk) != Group.CompressedSize)
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderGroupSize");
    const Decompressor* Codec = FindShaderCodec(Provider);
    std::size_t Slack = Codec != nullptr ? Codec->getInputSlack() : 0u;
    std::vector<std::byte> GroupBytes(Group.CompressedSize + Slack);
    Result<void> Read = Files.readChunk(*Chunk, 0, std::span(GroupBytes).first(Group.CompressedSize));
    if (!Read)
        return std::unexpected(Read.error());
    if (Group.IsGroupCompressed()) {
        std::vector<std::byte> Uncompressed;
        Result<void> Decompressed = Decompress(std::span(GroupBytes).first(Group.CompressedSize), Uncompressed, Group.UncompressedSize);
        if (!Decompressed)
            return std::unexpected(Decompressed.error());
        GroupBytes = std::move(Uncompressed);
    }
    return std::vector<std::byte>(GroupBytes.begin() + static_cast<std::ptrdiff_t>(Offset), GroupBytes.begin() + static_cast<std::ptrdiff_t>(Offset + Size));
}

Result<void> FShaderCodeLibrary::Decompress(std::span<const std::byte> Input, std::vector<std::byte>& Output, std::size_t UncompressedSize) const {
    const Decompressor* Codec = FindShaderCodec(Provider);
    if (Codec == nullptr)
        return fail(ErrorCode::UnknownCompression, "FShaderCodeLibrary.Codec");
    Output.resize(UncompressedSize + Codec->getOutputSlack());
    Result<void> Decompressed = Codec->decompress(Input, std::span(Output).first(UncompressedSize));
    if (!Decompressed)
        return Decompressed;
    Output.resize(UncompressedSize);
    return {};
}

}
