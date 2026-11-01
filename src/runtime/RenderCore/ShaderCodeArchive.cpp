#include "ue5parse/runtime/RenderCore/ShaderCodeArchive.h"

#include "ue5parse/compress/Decompressor.h"
#include "ue5parse/provider/FileProvider.h"

namespace ue5parse::runtime {

// UE's ShaderCodeArchive.cpp and ShaderCodeLibrary.cpp: reading the shader libraries (UE 5.3).

namespace {

// GShaderCodeArchiveVersion and FIoStoreShaderCodeArchive::CurrentVersion.
constexpr uint32 ShaderCodeArchiveVersion = 2u;
constexpr uint32 IoStoreShaderCodeArchiveVersion = 1u;
// The header of a library is read in growing ranges from this size, so that the code of a pak library is not read.
constexpr uint64 InitialHeaderReadSize = 1u << 20;

bool IsShaderLibraryName(std::string_view Name) {
    static constexpr std::string_view Extension = ".ushaderbytecode";
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

}

FArchive& operator<<(FArchive& Ar, FShaderMapEntry& Value) {
    return Ar << Value.ShaderIndicesOffset << Value.NumShaders << Value.FirstPreloadIndex << Value.NumPreloadEntries;
}

FArchive& operator<<(FArchive& Ar, FShaderCodeEntry& Value) { return Ar << Value.Offset << Value.Size << Value.UncompressedSize << Value.Frequency; }

FArchive& operator<<(FArchive& Ar, FFileCachePreloadEntry& Value) { return Ar << Value.Offset << Value.Size; }

FArchive& operator<<(FArchive& Ar, FIoStoreShaderMapEntry& Value) { return Ar << Value.ShaderIndicesOffset << Value.NumShaders; }

FArchive& operator<<(FArchive& Ar, FIoStoreShaderCodeEntry& Value) { return Ar << Value.Packed; }

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
    if (!Ar.IsError() && (ShaderHashes.Num() != ShaderEntries.Num() || ShaderMapHashes.Num() != ShaderMapEntries.Num() ||
                          ShaderGroupIoHashes.Num() != ShaderGroupEntries.Num()))
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
        if (!IsShaderLibraryName(File.name))
            continue;
        Result<void> Opened = OpenLibrary(File);
        if (!Opened)
            return Opened;
    }
    return {};
}

Result<void> FShaderCodeLibrary::OpenLibrary(const FileEntry& File) {
    const FileSystem& Files = Provider.getFiles();
    uint64 FileSize = Files.getFileSize(File);
    FLibrary Library;
    Library.File = File;
    std::vector<std::byte> Bytes;
    for (uint64 ReadSize = std::min(InitialHeaderReadSize, FileSize);; ReadSize = std::min(ReadSize * 2, FileSize)) {
        Bytes.resize(ReadSize);
        Result<void> Read = Files.readFile(File, 0, Bytes);
        if (!Read)
            return Read;
        FMemoryReaderView Ar(Bytes);
        Ar.SetGame(Provider.getGame());
        Ar << Library.Version;
        if (Library.Version == ShaderCodeArchiveVersion) {
            Library.PakArchive.Serialize(Ar);
            Library.LibraryCodeOffset = Ar.Tell();
        } else if (Library.Version == IoStoreShaderCodeArchiveVersion) {
            Library.IoStoreHeader.Serialize(Ar);
        } else if (!Ar.IsError()) {
            return fail(ErrorCode::UnsupportedVersion, "FShaderCodeLibrary.Version");
        }
        if (!Ar.IsError())
            break;
        if (Ar.GetError().code != ErrorCode::ReadPastEnd || ReadSize == FileSize)
            return std::unexpected(Ar.GetError());
    }

    uint32 LibraryIndex = static_cast<uint32>(Libraries.size());
    const TArray<FSHAHash>& ShaderMapHashes =
        Library.Version == ShaderCodeArchiveVersion ? Library.PakArchive.ShaderMapHashes : Library.IoStoreHeader.ShaderMapHashes;
    ShaderMaps.reserve(ShaderMaps.size() + static_cast<std::size_t>(ShaderMapHashes.Num()));
    for (int32 Index = 0; Index < ShaderMapHashes.Num(); Index++)
        ShaderMaps.try_emplace(ShaderMapHashes[Index], FShaderMapLocation{LibraryIndex, static_cast<uint32>(Index)});
    Libraries.push_back(std::move(Library));
    return {};
}

Result<std::vector<std::byte>> FShaderCodeLibrary::ReadShaderCode(const FSHAHash& ShaderMapHash, int32 ResourceIndex) const {
    auto Found = ShaderMaps.find(ShaderMapHash);
    if (Found == ShaderMaps.end())
        return fail(ErrorCode::MissingChunk, "FShaderCodeLibrary.ShaderMap");
    const FLibrary& Library = Libraries[Found->second.Library];
    int32 ShaderMapIndex = static_cast<int32>(Found->second.ShaderMap);
    uint32 ShaderIndicesOffset = 0u;
    uint32 NumShaders = 0u;
    if (Library.Version == ShaderCodeArchiveVersion) {
        ShaderIndicesOffset = Library.PakArchive.ShaderMapEntries[ShaderMapIndex].ShaderIndicesOffset;
        NumShaders = Library.PakArchive.ShaderMapEntries[ShaderMapIndex].NumShaders;
    } else {
        ShaderIndicesOffset = Library.IoStoreHeader.ShaderMapEntries[ShaderMapIndex].ShaderIndicesOffset;
        NumShaders = Library.IoStoreHeader.ShaderMapEntries[ShaderMapIndex].NumShaders;
    }
    if (ResourceIndex < 0 || static_cast<uint32>(ResourceIndex) >= NumShaders)
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ResourceIndex");
    const TArray<uint32>& ShaderIndices =
        Library.Version == ShaderCodeArchiveVersion ? Library.PakArchive.ShaderIndices : Library.IoStoreHeader.ShaderIndices;
    int32 IndexIndex = static_cast<int32>(ShaderIndicesOffset + static_cast<uint32>(ResourceIndex));
    if (!ShaderIndices.IsValidIndex(IndexIndex))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderIndices");
    uint32 ShaderIndex = ShaderIndices[IndexIndex];
    if (Library.Version == ShaderCodeArchiveVersion)
        return ReadPakShader(Library, ShaderIndex);
    return ReadIoStoreShader(Library, ShaderIndex);
}

Result<std::vector<std::byte>> FShaderCodeLibrary::ReadPakShader(const FLibrary& Library, uint32 ShaderIndex) const {
    if (!Library.PakArchive.ShaderEntries.IsValidIndex(static_cast<int32>(ShaderIndex)))
        return fail(ErrorCode::InvalidData, "FShaderCodeLibrary.ShaderEntries");
    const FShaderCodeEntry& Entry = Library.PakArchive.ShaderEntries[static_cast<int32>(ShaderIndex)];
    const FileSystem& Files = Provider.getFiles();
    const Decompressor* Oodle = Files.getCodecs().find(FName("Oodle"));
    std::size_t Slack = Oodle != nullptr ? Oodle->getInputSlack() : 0u;
    std::vector<std::byte> Code(Entry.Size + Slack);
    Result<void> Read = Files.readFile(Library.File, static_cast<uint64>(Library.LibraryCodeOffset) + Entry.Offset, std::span(Code).first(Entry.Size));
    if (!Read)
        return std::unexpected(Read.error());
    if (Entry.Size == Entry.UncompressedSize) {
        Code.resize(Entry.Size);
        return Code;
    }
    std::vector<std::byte> Uncompressed;
    Result<void> Decompressed = Decompress(std::span(Code).first(Entry.Size), Uncompressed, Entry.UncompressedSize);
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
    const Decompressor* Oodle = Files.getCodecs().find(FName("Oodle"));
    std::size_t Slack = Oodle != nullptr ? Oodle->getInputSlack() : 0u;
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
    return std::vector<std::byte>(GroupBytes.begin() + static_cast<std::ptrdiff_t>(Offset),
                                  GroupBytes.begin() + static_cast<std::ptrdiff_t>(Offset + Size));
}

Result<void> FShaderCodeLibrary::Decompress(std::span<const std::byte> Input, std::vector<std::byte>& Output, std::size_t UncompressedSize) const {
    // The shader code of UE 5.3 is always Oodle compressed (GetShaderCompressionFormat).
    const Decompressor* Oodle = Provider.getFiles().getCodecs().find(FName("Oodle"));
    if (Oodle == nullptr)
        return fail(ErrorCode::UnknownCompression, "FShaderCodeLibrary.Oodle");
    Output.resize(UncompressedSize + Oodle->getOutputSlack());
    Result<void> Decompressed = Oodle->decompress(Input, std::span(Output).first(UncompressedSize));
    if (!Decompressed)
        return Decompressed;
    Output.resize(UncompressedSize);
    return {};
}

}
