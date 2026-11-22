#pragma once

#include <array>
#include <string_view>
#include <unordered_map>

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "AoCDBCReader.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAoCDataChunk {
    GENERATED_BODY()

    UPROPERTY()
    uint64 Hash = 0;
    UPROPERTY()
    int64 Offset = 0;
    UPROPERTY()
    int64 Size = 0;
    UPROPERTY()
    int32 FileCount = 0;
    UPROPERTY()
    uint32 Checksum = 0;
};

struct FAoCChunkType {
    uint64 Hash;
    std::string_view Name;
};

//! Sorted by the hash.
extern const std::array<FAoCChunkType, 398> AoCChunkTypes;

USTRUCT()
struct FAoCFile {
    GENERATED_BODY()

    UPROPERTY()
    FString Name;
    UPROPERTY()
    FInstancedStruct Properties;
};

//! Reads the CacheDB file of Ashes of Creation: records in raw property mode, with names as offsets into a string table.
class FAoCDBCReader : public FMemoryReaderView {
public:
    explicit FAoCDBCReader(std::span<const std::byte> InBytes);

    using FArchive::operator<<;
    FArchive& operator<<(FName& Value) override;
    FArchive& operator<<(UObject*& Value) override;
    FArchive& operator<<(FSoftObjectPath& Value) override;

    const TArray<FAoCDataChunk>& GetChunks() const { return Chunks; }
    //! Reads all files of a chunk; false when the chunk type is not known or a record does not read.
    bool TryReadChunk(int32 Index, FString& Category, TArray<FAoCFile>& Files);

private:
    bool ReadFile(const FString& Type, FAoCFile& File);

    TArray<FAoCDataChunk> Chunks;
    std::unordered_map<int32, FName> NameMap;
};

}
