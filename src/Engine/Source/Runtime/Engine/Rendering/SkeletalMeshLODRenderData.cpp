#include "Engine/Source/Runtime/Engine/Rendering/SkeletalMeshLODRenderData.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "CppUE5Parse/Core/ByteReader.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/Rendering/SkeletalMeshRenderData.h"

namespace ue5parse::runtime {

namespace {

constexpr uint8 CDSF_AdjacencyData = 1;

void SkipClothVertexBuffer(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    SkipBulkSerializedArray(Ar);
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(12)) * 12);
}

void SkipSkinWeightProfiles(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(8);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FName Name;
        Ar << Name;
        Ar.SkipBytes(Ar.ReadCount(1));
        Ar.SkipBytes(Ar.ReadCount(1));
        Ar.SkipBytes(1);
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(8)) * 8);
    }
}

}

void FMultiSizeIndexContainer::Serialize(FArchive& Ar) {
    Ar << DataTypeSize;
    int32 ElementSize = 0;
    Ar << ElementSize;
    const int32 Count = Ar.ReadCount(DataTypeSize == 4 ? 4 : 2);
    if (Ar.IsError())
        return;
    if (Count > 0 && ElementSize != (DataTypeSize == 4 ? 4 : 2)) {
        Ar.SetError(ErrorCode::InvalidData, "FMultiSizeIndexContainer.ElementSize");
        return;
    }
    Indices.Empty(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        if (DataTypeSize == 4) {
            uint32 Value = 0;
            Ar << Value;
            Indices.Add(Value);
        } else {
            uint16 Value = 0;
            Ar << Value;
            Indices.Add(Value);
        }
    }
}

void FSkinWeightVertexBuffer::Serialize(FArchive& Ar) {
    FStripDataFlags DataFlags(Ar);
    Ar << bVariableBonesPerVertex << MaxBoneInfluences << NumBones << NumVertices << bUse16BitBoneIndex << bUse16BitBoneWeight;
    if (Ar.IsError())
        return;
    if (MaxBoneInfluences > 12) {
        Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.MaxBoneInfluences");
        return;
    }
    std::span<const std::byte> Data;
    if (!DataFlags.IsAudioVisualDataStripped()) {
        int32 ElementSize = 0;
        Ar << ElementSize;
        const int32 Size = Ar.ReadCount(1);
        Data = Ar.ReadView(Size);
        if (Size > 0 && ElementSize != 1)
            Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.Data");
    }
    FStripDataFlags LookupFlags(Ar);
    int32 NumLookupVertices = 0;
    Ar << NumLookupVertices;
    TArray<uint32> Lookup;
    if (!LookupFlags.IsAudioVisualDataStripped()) {
        int32 ElementSize = 0;
        Ar << ElementSize;
        const int32 Count = Ar.ReadCount(4);
        SerializeRaw(Ar, Lookup, Count);
        if (Count > 0 && ElementSize != 4)
            Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.Lookup");
    }
    if (Ar.IsError() || Data.empty())
        return;
    if (bVariableBonesPerVertex && static_cast<uint32>(Lookup.Num()) != NumVertices) {
        Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.LookupCount");
        return;
    }
    if (static_cast<uint64>(NumVertices) * MaxBoneInfluences > Data.size()) {
        Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.NumVertices");
        return;
    }
    const int32 Total = static_cast<int32>(NumVertices * MaxBoneInfluences);
    BoneIndices.Empty(Total);
    BoneWeights.Empty(Total);
    BoneIndices.SetNum(Total);
    BoneWeights.SetNum(Total);
    ByteReader Weights(Data);
    for (uint32 Vertex = 0; Vertex < NumVertices; Vertex++) {
        uint32 Influences = MaxBoneInfluences;
        if (bVariableBonesPerVertex) {
            Weights.seek(Lookup[static_cast<int32>(Vertex)] >> 8);
            Influences = std::min<uint32>(Lookup[static_cast<int32>(Vertex)] & 0xFF, MaxBoneInfluences);
        }
        const int32 Base = static_cast<int32>(Vertex * MaxBoneInfluences);
        for (uint32 Slot = 0; Slot < Influences; Slot++)
            BoneIndices[Base + static_cast<int32>(Slot)] = bUse16BitBoneIndex ? Weights.read<uint16>() : Weights.read<uint8>();
        for (uint32 Slot = 0; Slot < Influences; Slot++)
            BoneWeights[Base + static_cast<int32>(Slot)] = bUse16BitBoneWeight ? Weights.read<uint16>() : Weights.read<uint8>();
    }
    if (Weights.hasFailed())
        Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.Weights");
}

FArchive& operator<<(FArchive& Ar, FSkelMeshRenderSection& Section) {
    FStripDataFlags StripFlags(Ar);
    Ar << Section.MaterialIndex << Section.BaseIndex << Section.NumTriangles << Section.bRecomputeTangent;
    Ar << Section.RecomputeTangentsVertexMaskChannel << Section.bCastShadow << Section.bVisibleInRayTracing << Section.BaseVertexIndex;
    const int32 ClothLODCount = Ar.ReadCount(4);
    for (int32 i = 0; i < ClothLODCount && !Ar.IsError(); i++) {
        const int32 Count = Ar.ReadCount(64);
        if (Count > 0)
            Section.bHasClothingData = true;
        Ar.SkipBytes(static_cast<int64>(Count) * 64);
    }
    Ar << Section.BoneMap << Section.NumVertices << Section.MaxBoneInfluences << Section.CorrespondClothAssetIndex;
    Ar << Section.ClothingDataAssetGuid << Section.ClothingDataAssetLODIndex;
    if (!StripFlags.IsClassDataStripped(CDSF_AdjacencyData)) {
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(8)) * 8);
    }
    Ar << Section.bDisabled;
    return Ar;
}

bool FSkeletalMeshLODRenderData::HasClothData() const {
    for (const FSkelMeshRenderSection& Section : RenderSections)
        if (Section.bHasClothingData)
            return true;
    return false;
}

void FSkeletalMeshLODRenderData::SerializeStreamedData(FArchive& Ar, bool bHasVertexColors) {
    FStripDataFlags StripFlags(Ar);
    MultiSizeIndexContainer.Serialize(Ar);
    StaticVertexBuffers.PositionVertexBuffer.Serialize(Ar);
    StaticVertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    SkinWeightVertexBuffer.Serialize(Ar);
    if (bHasVertexColors)
        StaticVertexBuffers.ColorVertexBuffer.Serialize(Ar);
    if (HasClothData())
        SkipClothVertexBuffer(Ar);
    SkipSkinWeightProfiles(Ar);
    Ar.SkipBytes(Ar.ReadCount(1));
    bool bHasMorphData = false;
    Ar << bHasMorphData;
    if (bHasMorphData) {
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(16)) * 16);
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(16)) * 16);
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
        Ar.SkipBytes(4 + 4 + 4);
    }
    const int32 AttributeCount = Ar.ReadCount(24);
    for (int32 i = 0; i < AttributeCount && !Ar.IsError(); i++) {
        FName AttributeName;
        Ar << AttributeName;
        Ar.SkipBytes(4 * 4);
    }
    bHasBuffers = !Ar.IsError();
}

void FSkeletalMeshLODRenderData::SerializeAvailabilityInfo(FArchive& Ar) {
    int64 Size = 1 + 4;
    Size += 4 * 4;
    Size += 4 * 2;
    Size += 4 * 2;
    Size += 4 * 4 + 4 + 4 + 4;
    Ar.SkipBytes(Size);
    if (HasClothData()) {
        const int32 Count = Ar.ReadCount(8);
        Ar.SkipBytes(static_cast<int64>(Count) * 8 + 2 * 4 + static_cast<int64>(Count) * 4);
    }
    const int32 Profiles = Ar.ReadCount(8);
    for (int32 i = 0; i < Profiles && !Ar.IsError(); i++) {
        FName ProfileName;
        Ar << ProfileName;
    }
}

void FSkeletalMeshLODRenderData::Serialize(FArchive& Ar, bool bHasVertexColors) {
    FStripDataFlags StripFlags(Ar);
    Ar << bIsLODOptional << bStreamedDataInlined << RequiredBones;
    if (StripFlags.IsAudioVisualDataStripped() || bIsLODOptional)
        return;
    Ar << RenderSections << ActiveBoneIndices;
    uint32 BuffersSize = 0;
    Ar << BuffersSize;
    if (Ar.IsError())
        return;
    if (bStreamedDataInlined) {
        SerializeStreamedData(Ar, bHasVertexColors);
        return;
    }
    FByteBulkData BulkData;
    BulkData.Serialize(Ar);
    if (BulkData.GetElementCount() > 0) {
        Result<std::vector<std::byte>> Bytes = BulkData.Read();
        if (Bytes) {
            FMemoryReaderView BufferAr(*Bytes);
            BufferAr.CopyStateFrom(Ar);
            SerializeStreamedData(BufferAr, bHasVertexColors);
            if (BufferAr.IsError())
                bHasBuffers = false;
        }
        SerializeAvailabilityInfo(Ar);
    }
}

void FSkeletalMeshRenderData::Serialize(FArchive& Ar, bool bHasVertexColors) {
    const int32 LODCount = Ar.ReadCount(16);
    LODRenderData.Empty(LODCount);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++)
        LODRenderData[LODRenderData.AddDefaulted()].Serialize(Ar, bHasVertexColors);
    Ar << NumInlinedLODs << NumNonOptionalLODs;
}

}
