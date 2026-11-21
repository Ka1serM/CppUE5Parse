#include "Engine/Source/Runtime/GeometryCore/Util/DynamicVector.h"

#include <vector>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

namespace {

constexpr int64 PACKAGE_FILE_TAG = 0x9E2A83C1;
constexpr int64 ARCHIVE_V2_HEADER_TAG = PACKAGE_FILE_TAG | (static_cast<int64>(0x22222222) << 32);
constexpr int64 LOADING_COMPRESSION_CHUNK_SIZE = 131072;

struct FCompressedChunkInfo {
    int64 CompressedSize = 0;
    int64 UncompressedSize = 0;
};

FArchive& operator<<(FArchive& Ar, FCompressedChunkInfo& Info) {
    return Ar << Info.CompressedSize << Info.UncompressedSize;
}

FName SerializeCompressorName(FArchive& Ar) {
    uint8 CompressorNum = 0;
    Ar << CompressorNum;
    switch (CompressorNum) {
    case 0: {
        FString LoadedString;
        Ar << LoadedString;
        return MakeName(LoadedString);
    }
    case 1:
        return FName();
    case 2:
        return FName("Oodle");
    case 3:
        return FName("Zlib");
    case 4:
        return FName("Gzip");
    case 5:
        return FName("LZ4");
    default:
        Ar.SetError(ErrorCode::UnknownCompression, "FCompressionUtil.SerializeCompressorName");
        return FName();
    }
}

bool UncompressMemory(FArchive& Ar, FName Format, std::span<std::byte> Output, int64 CompressedSize) {
    if (Format.isNone()) {
        if (CompressedSize != static_cast<int64>(Output.size())) {
            Ar.SetError(ErrorCode::InvalidData, "FArchive.SerializeCompressedNew.Stored");
            return false;
        }
        Ar.Serialize(Output.data(), CompressedSize);
        return !Ar.IsError();
    }
    const Decompressor* Codec = GetRuntimeProvider().getFiles().getCodecs().find(Format);
    if (Codec == nullptr) {
        Ar.SetError(ErrorCode::UnknownCompression, "FArchive.SerializeCompressedNew.Format");
        return false;
    }
    std::vector<std::byte> Compressed(static_cast<std::size_t>(CompressedSize) + Codec->getInputSlack());
    Ar.Serialize(Compressed.data(), CompressedSize);
    if (Ar.IsError())
        return false;
    std::vector<std::byte> Uncompressed(Output.size() + Codec->getOutputSlack());
    if (!Codec->decompress(std::span(Compressed).first(static_cast<std::size_t>(CompressedSize)), std::span(Uncompressed).first(Output.size()))) {
        Ar.SetError(ErrorCode::Decompression, "FArchive.SerializeCompressedNew.Chunk");
        return false;
    }
    std::copy_n(Uncompressed.begin(), Output.size(), Output.begin());
    return true;
}

}

void SerializeCompressedNew(FArchive& Ar, std::span<std::byte> Output) {
    FCompressedChunkInfo PackageFileTag;
    Ar << PackageFileTag;
    FName CompressionFormatToDecode("Oodle");
    if (PackageFileTag.CompressedSize == ARCHIVE_V2_HEADER_TAG)
        CompressionFormatToDecode = SerializeCompressorName(Ar);
    else if (PackageFileTag.CompressedSize != PACKAGE_FILE_TAG)
        Ar.SetError(ErrorCode::BadMagic, "FArchive.SerializeCompressedNew.Tag");
    FCompressedChunkInfo Summary;
    Ar << Summary;
    if (Ar.IsError())
        return;
    const int64 LoadingCompressionChunkSize = PackageFileTag.UncompressedSize == PACKAGE_FILE_TAG ? LOADING_COMPRESSION_CHUNK_SIZE : PackageFileTag.UncompressedSize;
    if (LoadingCompressionChunkSize <= 0 || Summary.UncompressedSize != static_cast<int64>(Output.size()) || Summary.CompressedSize < 0 ||
        Summary.CompressedSize > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::InvalidData, "FArchive.SerializeCompressedNew.Summary");
        return;
    }
    const int64 TotalChunkCount = (Summary.UncompressedSize + LoadingCompressionChunkSize - 1) / LoadingCompressionChunkSize;
    TArray<FCompressedChunkInfo> CompressionChunks;
    int64 TotalChunkCompressedSize = 0;
    int64 TotalChunkUncompressedSize = 0;
    for (int64 ChunkIndex = 0; ChunkIndex < TotalChunkCount && !Ar.IsError(); ChunkIndex++) {
        FCompressedChunkInfo& Chunk = CompressionChunks[CompressionChunks.AddDefaulted()];
        Ar << Chunk;
        if (Chunk.CompressedSize < 0 || Chunk.UncompressedSize < 0)
            Ar.SetError(ErrorCode::InvalidData, "FArchive.SerializeCompressedNew.ChunkSize");
        TotalChunkCompressedSize += Chunk.CompressedSize;
        TotalChunkUncompressedSize += Chunk.UncompressedSize;
    }
    if (Ar.IsError())
        return;
    if (TotalChunkCompressedSize != Summary.CompressedSize || TotalChunkUncompressedSize != Summary.UncompressedSize) {
        Ar.SetError(ErrorCode::InvalidData, "FArchive.SerializeCompressedNew.Totals");
        return;
    }
    std::size_t Offset = 0;
    for (const FCompressedChunkInfo& Chunk : CompressionChunks) {
        if (!UncompressMemory(Ar, CompressionFormatToDecode, Output.subspan(Offset, static_cast<std::size_t>(Chunk.UncompressedSize)), Chunk.CompressedSize))
            return;
        Offset += static_cast<std::size_t>(Chunk.UncompressedSize);
    }
}

}
