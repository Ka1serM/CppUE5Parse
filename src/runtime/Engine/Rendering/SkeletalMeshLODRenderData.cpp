#include "ue5parse/runtime/Engine/Rendering/SkeletalMeshLODRenderData.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "ue5parse/core/ByteReader.h"
#include "ue5parse/runtime/CoreUObject/Serialization/BulkData.h"
#include "ue5parse/runtime/Engine/EngineUtils.h"
#include "ue5parse/runtime/Engine/Rendering/SkeletalMeshRenderData.h"

// The cooked LOD render data of skeletal meshes (UE's SkeletalMeshLODRenderData.cpp, SkeletalMeshRenderData.cpp,
// SkinWeightVertexBuffer.cpp, UE 5.3).

namespace ue5parse::runtime {

namespace {

// FSkeletalMeshLODRenderData's class strip flags.
constexpr uint8 CDSF_AdjacencyData = 1;

void SkipClothVertexBuffer(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped()) return;
    SkipBulkSerializedArray(Ar);                               // MeshToMeshVertData
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(12)) * 12);   // ClothIndexMapping
}

void SkipSkinWeightProfiles(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(8);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FName Name;
        Ar << Name;
        Ar.SkipBytes(Ar.ReadCount(1));                            // BoneIDs
        Ar.SkipBytes(Ar.ReadCount(1));                            // BoneWeights
        Ar.SkipBytes(1);                                          // NumWeightsPerVertex
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(8)) * 8);    // VertexIndexToInfluenceOffset
    }
}

}

void FMultiSizeIndexContainer::Serialize(FArchive& Ar) {
    Ar << DataTypeSize;
    int32 ElementSize = 0;
    Ar << ElementSize;
    const int32 Count = Ar.ReadCount(DataTypeSize == 4 ? 4 : 2);
    if (Ar.IsError()) return;
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
    if (Ar.IsError()) return;
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
        if (Size > 0 && ElementSize != 1) Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.Data");
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
        if (Count > 0 && ElementSize != 4) Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.Lookup");
    }
    if (Ar.IsError() || Data.empty()) return;
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
            // The lookup has the offset in the high 24 bits and the influence count in the low 8.
            Weights.seek(Lookup[static_cast<int32>(Vertex)] >> 8);
            Influences = std::min<uint32>(Lookup[static_cast<int32>(Vertex)] & 0xFF, MaxBoneInfluences);
        }
        const int32 Base = static_cast<int32>(Vertex * MaxBoneInfluences);
        for (uint32 Slot = 0; Slot < Influences; Slot++)
            BoneIndices[Base + static_cast<int32>(Slot)] = bUse16BitBoneIndex ? Weights.read<uint16>() : Weights.read<uint8>();
        for (uint32 Slot = 0; Slot < Influences; Slot++)
            BoneWeights[Base + static_cast<int32>(Slot)] = bUse16BitBoneWeight ? Weights.read<uint16>() : Weights.read<uint8>();
    }
    if (Weights.hasFailed()) Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.Weights");
}

FArchive& operator<<(FArchive& Ar, FSkelMeshRenderSection& Section) {
    FStripDataFlags StripFlags(Ar);
    Ar << Section.MaterialIndex << Section.BaseIndex << Section.NumTriangles << Section.bRecomputeTangent;
    Ar << Section.RecomputeTangentsVertexMaskChannel << Section.bCastShadow << Section.bVisibleInRayTracing << Section.BaseVertexIndex;
    // ClothMappingDataLODs: an array of arrays of FMeshToMeshVertData (64 bytes each).
    const int32 ClothLODCount = Ar.ReadCount(4);
    for (int32 i = 0; i < ClothLODCount && !Ar.IsError(); i++) {
        const int32 Count = Ar.ReadCount(64);
        if (Count > 0) Section.bHasClothingData = true;
        Ar.SkipBytes(static_cast<int64>(Count) * 64);
    }
    Ar << Section.BoneMap << Section.NumVertices << Section.MaxBoneInfluences << Section.CorrespondClothAssetIndex;
    Ar << Section.ClothingDataAssetGuid << Section.ClothingDataAssetLODIndex;
    if (!StripFlags.IsClassDataStripped(CDSF_AdjacencyData)) {
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);   // DuplicatedVerticesBuffer.DupVertData
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(8)) * 8);   // DupVertIndexData
    }
    Ar << Section.bDisabled;
    return Ar;
}

int32 FSkeletalMeshLODRenderData::GetTotalFaces() const {
    int32 Total = 0;
    for (const FSkelMeshRenderSection& Section : RenderSections) Total += static_cast<int32>(Section.NumTriangles);
    return Total;
}

void FSkeletalMeshLODRenderData::GetSectionFromVertexIndex(int32 InVertIndex, int32& OutSectionIndex, int32& OutVertIndex) const {
    OutSectionIndex = 0;
    OutVertIndex = 0;
    int32 VertCount = 0;
    for (int32 SectionIndex = 0; SectionIndex < RenderSections.Num(); SectionIndex++) {
        const FSkelMeshRenderSection& Section = RenderSections[SectionIndex];
        if (InVertIndex < VertCount + static_cast<int32>(Section.NumVertices)) {
            OutSectionIndex = SectionIndex;
            OutVertIndex = InVertIndex - VertCount;
            return;
        }
        VertCount += static_cast<int32>(Section.NumVertices);
    }
}

bool FSkeletalMeshLODRenderData::HasClothData() const {
    for (const FSkelMeshRenderSection& Section : RenderSections)
        if (Section.bHasClothingData) return true;
    return false;
}

void FSkeletalMeshLODRenderData::SerializeStreamedData(FArchive& Ar, bool bHasVertexColors) {
    FStripDataFlags StripFlags(Ar);
    MultiSizeIndexContainer.Serialize(Ar);
    StaticVertexBuffers.PositionVertexBuffer.Serialize(Ar);
    StaticVertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    SkinWeightVertexBuffer.Serialize(Ar);
    if (bHasVertexColors) StaticVertexBuffers.ColorVertexBuffer.Serialize(Ar);
    if (HasClothData()) SkipClothVertexBuffer(Ar);
    SkipSkinWeightProfiles(Ar);
    Ar.SkipBytes(Ar.ReadCount(1));   // RayTracingData
    // The compressed morph target buffers.
    bool bHasMorphData = false;
    Ar << bHasMorphData;
    if (bHasMorphData) {
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);    // MorphData
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(16)) * 16);  // MinimumValuePerMorph
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(16)) * 16);  // MaximumValuePerMorph
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);    // BatchStartOffsetPerMorph
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);    // BatchesPerMorph
        Ar.SkipBytes(4 + 4 + 4);                                  // NumTotalBatches, PositionPrecision, TangentZPrecision
    }
    // VertexAttributeBuffers: name -> component count, pixel format, component stride, stride.
    const int32 AttributeCount = Ar.ReadCount(24);
    for (int32 i = 0; i < AttributeCount && !Ar.IsError(); i++) {
        FName AttributeName;
        Ar << AttributeName;
        Ar.SkipBytes(4 * 4);
    }
    bHasBuffers = !Ar.IsError();
}

void FSkeletalMeshLODRenderData::SerializeAvailabilityInfo(FArchive& Ar) {
    // The sizes of the buffers after a streamed LOD's bulk data.
    int64 Size = 1 + 4;            // FMultiSizeIndexContainer
    Size += 4 * 4;                 // FStaticMeshVertexBuffer
    Size += 4 * 2;                 // FPositionVertexBuffer
    Size += 4 * 2;                 // FColorVertexBuffer
    Size += 4 * 4 + 4 + 4 + 4;     // FSkinWeightVertexBuffer
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
    if (StripFlags.IsAudioVisualDataStripped() || bIsLODOptional) return;
    Ar << RenderSections << ActiveBoneIndices;
    uint32 BuffersSize = 0;
    Ar << BuffersSize;
    if (Ar.IsError()) return;
    if (bStreamedDataInlined) {
        SerializeStreamedData(Ar, bHasVertexColors);
        return;
    }
    // A streamed LOD: the buffers are in bulk data, their sizes follow it.
    FByteBulkData BulkData;
    BulkData.Serialize(Ar);
    if (BulkData.GetElementCount() > 0) {
        Result<std::vector<std::byte>> Bytes = BulkData.Read();
        if (Bytes) {
            FMemoryReaderView BufferAr(*Bytes);
            BufferAr.CopyStateFrom(Ar);
            SerializeStreamedData(BufferAr, bHasVertexColors);
            if (BufferAr.IsError()) bHasBuffers = false;
        }
        SerializeAvailabilityInfo(Ar);
    }
}

void FSkeletalMeshRenderData::Serialize(FArchive& Ar, bool bHasVertexColors) {
    const int32 LODCount = Ar.ReadCount(16);
    LODRenderData.Empty(LODCount);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++) LODRenderData[LODRenderData.AddDefaulted()].Serialize(Ar, bHasVertexColors);
    Ar << NumInlinedLODs << NumNonOptionalLODs;
}

int32 FSkeletalMeshRenderData::GetFirstValidLODIdx(int32 MinLODIdx) const {
    for (int32 LODIndex = FMath::Max(MinLODIdx, 0); LODIndex < LODRenderData.Num(); LODIndex++)
        if (LODRenderData[LODIndex].bHasBuffers) return LODIndex;
    return INDEX_NONE;
}

}
