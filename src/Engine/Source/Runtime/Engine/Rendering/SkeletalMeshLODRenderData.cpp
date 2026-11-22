#include "Engine/Source/Runtime/Engine/Rendering/SkeletalMeshLODRenderData.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "CppUE5Parse/Core/ByteReader.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/PackedNormal.h"
#include "Engine/Source/Runtime/Engine/Rendering/SkeletalMeshRenderData.h"

namespace ue5parse::runtime {

namespace {

constexpr uint8 CDSF_AdjacencyData = 1;
constexpr uint8 MinLodStripFlag = 2;
constexpr uint8 MeshDeformerStripFlag = 1;
constexpr int32 RayTracingRawDataHeaderSize = 6 * 4;
constexpr int32 MeshToMeshVertDataSize = 64;

constexpr uint32 MaxInfluencesPerStream = 4;
constexpr uint32 ExtraBoneInfluences = 8;

bool HasUnlimitedBoneInfluences(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::UnlimitedBoneInfluences, EGame::GAME_UE4_25);
}

bool HasBoneIndexSizeFlag(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::IncreaseBoneIndexLimitPerChunk, EGame::GAME_UE4_25);
}

//! UE 4.24 adds the vertex stride to the legacy skin weight metadata.
bool HasLegacySkinWeightStride(const FArchive& Ar) {
    return Ar.GetGame() >= EGame::GAME_UE4_24
           && Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::SplitModelAndRenderData, EGame::GAME_UE4_19);
}

void SkipArray(FArchive& Ar, int32 ElementSize) {
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(ElementSize)) * ElementSize);
}

bool HasTessellation(const FArchive& Ar) {
    return !Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::RemovingTessellation, EGame::GAME_UE5_0);
}

//! UE 5.6 stores an offline header before the ray tracing geometry data.
bool HasRayTracingDataHeader(const FArchive& Ar) {
    return Ar.GetGame() >= EGame::GAME_UE5_6;
}

//! UE 5.0 replaces the 64-bit cloth index mapping with FClothBufferIndexMapping.
int32 GetClothIndexMappingSize(const FArchive& Ar) {
    return Ar.GetGame() >= EGame::GAME_UE5_0 ? 12 : 8;
}

void SkipClothVertexBuffer(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    SkipBulkSerializedArray(Ar);
    SkipArray(Ar, GetClothIndexMappingSize(Ar));
}

void SkipSkinWeightProfiles(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(8);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FName Name;
        Ar << Name;
        SkipArray(Ar, 1);
        SkipArray(Ar, 1);
        Ar.SkipBytes(1);
        SkipArray(Ar, 8);
    }
}

void SkipMorphTargetVertexInfoBuffers(FArchive& Ar) {
    SkipArray(Ar, 4);
    SkipArray(Ar, 16);
    SkipArray(Ar, 16);
    SkipArray(Ar, 4);
    SkipArray(Ar, 4);
    Ar.SkipBytes(4 + 4 + 4);
}

void SkipVertexAttributeBuffers(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(8);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FName Name;
        Ar << Name;
        Ar.SkipBytes(4 + 4 + 4 + 4);
        SkipBulkSerializedArray(Ar);
    }
}

bool HasCombinedSectionsAndChunks(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::CombineSectionWithChunk, EGame::GAME_UE4_13);
}

bool HasCombinedSoftAndRigidVerts(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::CombineSoftAndRigidVerts, EGame::GAME_UE4_13);
}

bool HasSeparateSkinWeightBuffer(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::UseSeparateSkinWeightBuffer, EGame::GAME_UE4_15);
}

//! Skips the editor-only soft vertices, which cooked packages strip.
bool CheckSoftVerticesStripped(FArchive& Ar, const FStripDataFlags& StripFlags) {
    if (StripFlags.IsEditorDataStripped())
        return true;
    Ar.SetError(ErrorCode::UnsupportedVersion, "FSkelMeshSection.SoftVertices");
    return false;
}

//! Reads the APEX cloth mapping and the physical mesh of a section or legacy chunk.
bool SerializeLegacyClothMapping(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(MeshToMeshVertDataSize);
    Ar.SkipBytes(static_cast<int64>(Count) * MeshToMeshVertDataSize);
    SkipArray(Ar, 12);
    SkipArray(Ar, 12);
    return Count > 0;
}

//! Reads the FSkeletalMeshVertexColorBuffer of UE 4.14 and earlier.
void SerializeLegacyColorBuffer(FArchive& Ar, FColorVertexBuffer& ColorVertexBuffer) {
    FStripDataFlags ColorStripFlags(Ar, EUnrealEngineObjectUE4Version::STATIC_SKELETAL_MESH_SERIALIZATION_FIX);
    int32 SerializedElementSize = 0;
    Ar << SerializedElementSize;
    const int32 Count = Ar.ReadCount(4);
    TArray<FColor> Colors;
    SerializeRaw(Ar, Colors, Count);
    if (Count > 0 && SerializedElementSize != 4)
        Ar.SetError(ErrorCode::InvalidData, "FSkeletalMeshVertexColorBuffer.ElementSize");
    ColorVertexBuffer.Init(std::move(Colors));
}

//! Final Fantasy VII Remake stores an optional adjacency buffer and an optional second color buffer.
void SerializeFinalFantasy7RemakeBuffers(FArchive& Ar, FColorVertexBuffer& ColorVertexBuffer) {
    int32 CheckInt = 0;
    Ar << CheckInt;
    if (CheckInt >= 10) {
        Ar.Seek(Ar.Tell() - 4);
        FMultiSizeIndexContainer AdjacencyMultiSizeIndexContainer;
        AdjacencyMultiSizeIndexContainer.Serialize(Ar);
    }
    Ar << CheckInt;
    if (Ar.IsError() || CheckInt == 0 || CheckInt == 1)
        return;
    Ar.Seek(Ar.Tell() - 4);
    FStripDataFlags InternalStripFlags(Ar);
    if (InternalStripFlags.IsClassDataStripped(CDSF_AdjacencyData)) {
        Ar.Seek(Ar.Tell() - 2);
        return;
    }
    int32 Size = 0;
    int32 Count = 0;
    Ar << Size << Count;
    if (Count < 30) {
        Ar.Seek(Ar.Tell() - 10);
        return;
    }
    Ar.SkipBytes(static_cast<int64>(Size) * Count);
    SerializeLegacyColorBuffer(Ar, ColorVertexBuffer);
}

//! Sea of Thieves ends each FStaticLODModel with one more index buffer.
void SkipSeaOfThievesIndexContainer(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_SeaOfThieves)
        return;
    FMultiSizeIndexContainer IndexContainer;
    IndexContainer.Serialize(Ar);
}

void SerializeStaticLODSection(FArchive& Ar, FSkelMeshRenderSection& Section) {
    FStripDataFlags StripFlags(Ar);
    Ar << Section.MaterialIndex;
    const bool bCombined = HasCombinedSectionsAndChunks(Ar);
    if (!bCombined) {
        uint16 DummyChunkIndex = 0;
        Ar << DummyChunkIndex;
    }
    if (!StripFlags.IsAudioVisualDataStripped())
        Ar << Section.BaseIndex << Section.NumTriangles;
    uint8 TriangleSorting = 0;
    Ar << TriangleSorting;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::APEX_CLOTH) {
        int16 CorrespondClothSectionIndex = INDEX_NONE;
        Ar << Section.bDisabled << CorrespondClothSectionIndex;
    }
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::APEX_CLOTH_LOD) {
        uint8 bEnableClothLOD = 0;
        Ar << bEnableClothLOD;
    }
    if (Ar.GetGame() == EGame::GAME_DaysGone)
        return;
    if (Ar.CustomVerAtLeast(FRecomputeTangentCustomVersion::GUID, FRecomputeTangentCustomVersion::RuntimeRecomputeTangent, EGame::GAME_UE4_12))
        Ar << Section.bRecomputeTangent;
    if (Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::RefactorMeshEditorMaterials, EGame::GAME_UE4_14))
        Ar << Section.bCastShadow;
    if (Ar.GetGame() == EGame::GAME_TrainSimWorld2020)
        Ar.SkipBytes(8);
    if (!bCombined || Ar.IsError())
        return;
    if (!StripFlags.IsAudioVisualDataStripped())
        Ar << Section.BaseVertexIndex;
    if (!CheckSoftVerticesStripped(Ar, StripFlags))
        return;
    Ar << Section.BoneMap;
    if (Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::SaveNumVertices, EGame::GAME_UE4_13))
        Ar << Section.NumVertices;
    if (!HasCombinedSoftAndRigidVerts(Ar))
        Ar.SkipBytes(4 + 4);
    Ar << Section.MaxBoneInfluences;
    Section.bHasClothingData = SerializeLegacyClothMapping(Ar);
    Ar << Section.CorrespondClothAssetIndex;
    if (Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::NewClothingSystemAdded, EGame::GAME_UE4_16)) {
        Ar << Section.ClothingDataAssetGuid << Section.ClothingDataAssetLODIndex;
    } else {
        int16 DummyClothAssetSubmeshIndex = INDEX_NONE;
        Ar << DummyClothAssetSubmeshIndex;
    }
    if (Ar.GetGame() != EGame::GAME_KingdomHearts3 && Ar.GetGame() != EGame::GAME_FinalFantasy7Remake)
        return;
    int32 bShouldReadArray = 0;
    int32 ArrayLength = 0;
    Ar << bShouldReadArray << ArrayLength;
    if (bShouldReadArray == 1)
        Ar.SkipBytes(static_cast<int64>(ArrayLength) * (Ar.GetGame() == EGame::GAME_KingdomHearts3 ? 24 : 16));
}

//! Before UE 4.13 the bone data of a section lives in a separate chunk.
void SerializeLegacyChunk(FArchive& Ar, FSkelMeshRenderSection& Section) {
    FStripDataFlags StripFlags(Ar);
    if (!StripFlags.IsAudioVisualDataStripped())
        Ar << Section.BaseVertexIndex;
    if (!CheckSoftVerticesStripped(Ar, StripFlags))
        return;
    Ar << Section.BoneMap;
    if (!HasCombinedSoftAndRigidVerts(Ar))
        Ar.SkipBytes(4 + 4);
    Ar << Section.MaxBoneInfluences;
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::APEX_CLOTH)
        return;
    Section.bHasClothingData = SerializeLegacyClothMapping(Ar);
    int16 ClothAssetSubmeshIndex = INDEX_NONE;
    Ar << Section.CorrespondClothAssetIndex << ClothAssetSubmeshIndex;
}

void SerializeStaticLODSections(FArchive& Ar, TArray<FSkelMeshRenderSection>& Sections) {
    const int32 Count = Ar.ReadCount(4);
    Sections.Empty(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++)
        SerializeStaticLODSection(Ar, Sections[Sections.AddDefaulted()]);
}

void SkipLegacyClothVertexBuffer(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar, EUnrealEngineObjectUE4Version::STATIC_SKELETAL_MESH_SERIALIZATION_FIX);
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    SkipBulkSerializedArray(Ar);
    if (Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::CompactClothVertexBuffer, EGame::GAME_UE4_18))
        SkipArray(Ar, 8);
}

void SkipHalfEdgeBuffer(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsClassDataStripped(MeshDeformerStripFlag))
        return;
    SkipArray(Ar, 4);
    SkipArray(Ar, 4);
}

}

void FMultiSizeIndexContainer::Serialize(FArchive& Ar) {
    Ar << DataTypeSize;
    if (Ar.GetGame() == EGame::GAME_OutlastTrials)
        Ar.SkipBytes(4);
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

std::span<const std::byte> FSkinWeightVertexBuffer::SerializeLegacy(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    bool bExtraBoneInfluences = false;
    Ar << bExtraBoneInfluences;
    if (HasLegacySkinWeightStride(Ar)) {
        uint32 Stride = 0;
        Ar << Stride;
    }
    Ar << NumVertices;
    if (HasBoneIndexSizeFlag(Ar))
        Ar << bUse16BitBoneIndex;
    MaxBoneInfluences = bExtraBoneInfluences ? ExtraBoneInfluences : MaxInfluencesPerStream;
    NumBones = MaxBoneInfluences * NumVertices;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped())
        return {};
    const int32 ElementSize = static_cast<int32>(MaxBoneInfluences) * ((bUse16BitBoneIndex ? 2 : 1) + 1);
    //! UE 4.25 reads the weights of older assets per element, without an element size.
    const bool bPerElement = Ar.GetGame() >= EGame::GAME_UE4_25 && !HasBoneIndexSizeFlag(Ar);
    int32 SerializedElementSize = ElementSize;
    if (!bPerElement)
        Ar << SerializedElementSize;
    const int32 Count = Ar.ReadCount(ElementSize);
    std::span<const std::byte> Data = Ar.ReadView(static_cast<int64>(Count) * ElementSize);
    if (Count > 0 && SerializedElementSize != ElementSize)
        Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.ElementSize");
    return Data;
}

void FSkinWeightVertexBuffer::SerializeMetaData(FArchive& Ar) {
    if (!HasUnlimitedBoneInfluences(Ar)) {
        Ar.SkipBytes(4 + (HasLegacySkinWeightStride(Ar) ? 4 : 0) + 4 + (HasBoneIndexSizeFlag(Ar) ? 4 : 0));
        return;
    }
    Ar.SkipBytes(4 * 4 + 4);
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::IncreasedSkinWeightPrecision, EGame::GAME_UE5_2))
        Ar.SkipBytes(4);
    Ar.SkipBytes(4);
}

void FSkinWeightVertexBuffer::Init(TArray<uint16>&& InBoneIndices, TArray<uint16>&& InBoneWeights, uint32 InMaxBoneInfluences, uint32 InNumVertices) {
    bVariableBonesPerVertex = false;
    MaxBoneInfluences = InMaxBoneInfluences;
    NumVertices = InNumVertices;
    NumBones = InMaxBoneInfluences * InNumVertices;
    bUse16BitBoneIndex = false;
    bUse16BitBoneWeight = false;
    BoneIndices = std::move(InBoneIndices);
    BoneWeights = std::move(InBoneWeights);
}

void FSkinWeightVertexBuffer::Serialize(FArchive& Ar) {
    std::span<const std::byte> Data;
    TArray<uint32> Lookup;
    if (HasUnlimitedBoneInfluences(Ar))
        Data = SerializeData(Ar, Lookup);
    else
        Data = SerializeLegacy(Ar);
    if (Ar.IsError() || Data.empty())
        return;
    ReadWeights(Ar, Data, Lookup);
}

std::span<const std::byte> FSkinWeightVertexBuffer::SerializeData(FArchive& Ar, TArray<uint32>& Lookup) {
    FStripDataFlags DataFlags(Ar);
    Ar << bVariableBonesPerVertex << MaxBoneInfluences << NumBones << NumVertices << bUse16BitBoneIndex;
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::IncreasedSkinWeightPrecision, EGame::GAME_UE5_2))
        Ar << bUse16BitBoneWeight;
    if (Ar.IsError())
        return {};
    if (MaxBoneInfluences > 12) {
        Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.MaxBoneInfluences");
        return {};
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
    if (!LookupFlags.IsAudioVisualDataStripped()) {
        int32 ElementSize = 0;
        Ar << ElementSize;
        const int32 Count = Ar.ReadCount(4);
        SerializeRaw(Ar, Lookup, Count);
        if (Count > 0 && ElementSize != 4)
            Ar.SetError(ErrorCode::InvalidData, "FSkinWeightVertexBuffer.Lookup");
    }
    return Data;
}

void FSkinWeightVertexBuffer::ReadWeights(FArchive& Ar, std::span<const std::byte> Data, const TArray<uint32>& Lookup) {
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

namespace {

//! Bytes that games add at the end of a render section.
int64 GetRenderSectionGameDataSize(EGame Game) {
    switch (Game) {
    case EGame::GAME_OutlastTrials:
        return 1;
    case EGame::GAME_RogueCompany:
    case EGame::GAME_BladeAndSoul:
    case EGame::GAME_SYNCED:
    case EGame::GAME_StarWarsHunters:
        return 4;
    case EGame::GAME_FragPunk:
    case EGame::GAME_InfinityNikki:
        return 8;
    case EGame::GAME_MortalKombat1:
        return 12;
    case EGame::GAME_FateTrigger:
        return 19;
    case EGame::GAME_Strinova:
        return 18;
    case EGame::GAME_SuicideSquad:
        return 11;
    default:
        return 0;
    }
}

}

FArchive& operator<<(FArchive& Ar, FSkelMeshRenderSection& Section) {
    const EGame Game = Ar.GetGame();
    FStripDataFlags StripFlags(Ar);
    if (Game == EGame::GAME_Raven2)
        Ar.SkipBytes(4);
    Ar << Section.MaterialIndex << Section.BaseIndex << Section.NumTriangles;
    if (Game == EGame::GAME_Paragon)
        Ar.SkipBytes(1);
    Ar << Section.bRecomputeTangent;
    if (Ar.CustomVerAtLeast(FRecomputeTangentCustomVersion::GUID, FRecomputeTangentCustomVersion::RecomputeTangentVertexColorMask, EGame::GAME_UE4_26))
        Ar << Section.RecomputeTangentsVertexMaskChannel;
    if (Game == EGame::GAME_DeltaForce)
        Ar.SkipBytes(3);
    if (Game == EGame::GAME_BigRumbleBoxingCreedChampions)
        Ar.SkipBytes(4);
    Ar << Section.bCastShadow;
    if (Game == EGame::GAME_FinalFantasy7Rebirth || Game == EGame::GAME_HogwartsLegacy || Game == EGame::GAME_Snowbreak
        || Game == EGame::GAME_ChasingKaleidoRIDER)
        Ar.SkipBytes(4);
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::SkelMeshSectionVisibleInRayTracingFlagAdded, EGame::GAME_UE5_0))
        Ar << Section.bVisibleInRayTracing;
    Ar << Section.BaseVertexIndex;
    const bool bClothMappingLODs =
        Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::AddClothMappingLODBias, EGame::GAME_UE5_0);
    const int32 ClothLODCount = bClothMappingLODs ? Ar.ReadCount(4) : 1;
    for (int32 i = 0; i < ClothLODCount && !Ar.IsError(); i++) {
        const int32 Count = Ar.ReadCount(MeshToMeshVertDataSize);
        if (Count > 0)
            Section.bHasClothingData = true;
        Ar.SkipBytes(static_cast<int64>(Count) * MeshToMeshVertDataSize);
    }
    Ar << Section.BoneMap << Section.NumVertices << Section.MaxBoneInfluences;
    //! UE 5.8 packs bUnifiedBoneMap into the top bit of MaxBoneInfluences.
    if (Ar.GetGame() >= EGame::GAME_UE5_8) {
        constexpr uint32 UnifiedBoneMapBit = 1u << 31;
        const uint32 PackedBits = static_cast<uint32>(Section.MaxBoneInfluences);
        Section.bUnifiedBoneMap = (PackedBits & UnifiedBoneMapBit) != 0;
        Section.MaxBoneInfluences = static_cast<int32>(PackedBits & ~UnifiedBoneMapBit);
    }
    Ar << Section.CorrespondClothAssetIndex;
    Ar << Section.ClothingDataAssetGuid << Section.ClothingDataAssetLODIndex;
    if (Game == EGame::GAME_Paragon)
        return Ar;
    if (!StripFlags.IsClassDataStripped(CDSF_AdjacencyData)) {
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(8)) * 8);
    }
    Ar << Section.bDisabled;
    if (Game == EGame::GAME_InfinityNikki) {
        int32 Data = 0;
        Ar << Data;
        Section.CustomData = Data;
    }
    Ar.SkipBytes(GetRenderSectionGameDataSize(Game));
    return Ar;
}

namespace {

//! Buffers that games add after the inlined data of a LOD.
void SkipInlinedGameData(FArchive& Ar) {
    if (Ar.GetGame() == EGame::GAME_RogueCompany) {
        Ar.SkipBytes(12);
        int32 ElementSize = 0, ElementCount = 0;
        Ar << ElementSize << ElementCount;
        if (ElementSize > 0 && ElementCount > 0)
            SkipBulkSerializedArray(Ar);
        return;
    }
    if (Ar.GetGame() != EGame::GAME_MortalKombat1)
        return;
    bool bHasMorphTargetBuffer = false;
    Ar << bHasMorphTargetBuffer;
    if (!bHasMorphTargetBuffer)
        return;
    for (int32 i = 0; i < 7 && !Ar.IsError(); i++)
        SkipBulkSerializedArray(Ar);
    Ar.SkipBytes(8);
}

}

bool FSkeletalMeshLODRenderData::HasClothData() const {
    for (const FSkelMeshRenderSection& Section : RenderSections)
        if (Section.bHasClothingData)
            return true;
    return false;
}

bool FSkeletalMeshLODRenderData::HasInfinityNikkiSectionData() const {
    for (const FSkelMeshRenderSection& Section : RenderSections)
        if (Section.CustomData.value_or(0) > 0)
            return true;
    return false;
}

void FSkeletalMeshLODRenderData::SerializeStreamedData(FArchive& Ar, bool bHasVertexColors) {
    FStripDataFlags StripFlags(Ar);
    MultiSizeIndexContainer.Serialize(Ar);
    StaticVertexBuffers.PositionVertexBuffer.Serialize(Ar);
    StaticVertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    SkinWeightVertexBuffer.Serialize(Ar);
    const EGame Game = Ar.GetGame();
    if (Game == EGame::GAME_EvilWest)
        Ar.SkipBytes(22);
    if (bHasVertexColors)
        StaticVertexBuffers.ColorVertexBuffer.Serialize(Ar);
    if (HasTessellation(Ar) && !StripFlags.IsClassDataStripped(CDSF_AdjacencyData) && Game != EGame::GAME_GTATheTrilogyDefinitiveEdition
        && Game != EGame::GAME_FinalFantasy7Rebirth) {
        FMultiSizeIndexContainer AdjacencyMultiSizeIndexContainer;
        AdjacencyMultiSizeIndexContainer.Serialize(Ar);
    }
    if (HasClothData())
        SkipClothVertexBuffer(Ar);
    const int32 ExtraIndexContainers = Game == EGame::GAME_InfinityNikki && HasInfinityNikkiSectionData() ? 4 : Game == EGame::GAME_Spectre ? 1 : 0;
    for (int32 i = 0; i < ExtraIndexContainers; i++) {
        FMultiSizeIndexContainer Unused;
        Unused.Serialize(Ar);
    }
    SkipSkinWeightProfiles(Ar);
    if (HasRayTracingDataHeader(Ar)) {
        Ar.SkipBytes(RayTracingRawDataHeaderSize);
        SkipBulkSerializedArray(Ar);
    } else if (Ar.GetGame() >= EGame::GAME_UE4_27 || Ar.GetGame() == EGame::GAME_UE4_25_Plus) {
        SkipArray(Ar, 1);
    }
    if (Ar.CustomVerAtLeast(FUE5SpecialProjectStreamObjectVersion::GUID, FUE5SpecialProjectStreamObjectVersion::SerializeSkeletalMeshMorphTargetRenderData,
                            EGame::GAME_UE5_0)
        || Game == EGame::GAME_TheQuarry) {
        bool bSerializeCompressedMorphTargets = false;
        Ar << bSerializeCompressedMorphTargets;
        if (bSerializeCompressedMorphTargets)
            SkipMorphTargetVertexInfoBuffers(Ar);
    }
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::SkeletalVertexAttributes, EGame::GAME_UE5_3))
        SkipVertexAttributeBuffers(Ar);
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::SkeletalHalfEdgeData, EGame::GAME_UE5_4))
        SkipHalfEdgeBuffer(Ar);
    bHasBuffers = !Ar.IsError();
}

void FSkeletalMeshLODRenderData::SerializeAvailabilityInfo(FArchive& Ar, bool bAdjacencyDataStripped) {
    constexpr int64 IndexMetaDataSize = 1 + 4;
    Ar.SkipBytes(IndexMetaDataSize);
    if (HasTessellation(Ar) && !bAdjacencyDataStripped)
        Ar.SkipBytes(IndexMetaDataSize);
    Ar.SkipBytes(4 * 4 + 4 * 2 + 4 * 2);
    SkinWeightVertexBuffer.SerializeMetaData(Ar);
    if (Ar.GetGame() == EGame::GAME_StarWarsJediSurvivor)
        Ar.SkipBytes(4);
    if (HasClothData()) {
        SkipArray(Ar, GetClothIndexMappingSize(Ar));
        Ar.SkipBytes(4 + 4);
    }
    const int32 Profiles = Ar.ReadCount(8);
    for (int32 i = 0; i < Profiles && !Ar.IsError(); i++) {
        FName ProfileName;
        Ar << ProfileName;
    }
    if (HasRayTracingDataHeader(Ar))
        Ar.SkipBytes(RayTracingRawDataHeaderSize);
}

void FSkeletalMeshLODRenderData::SerializeGPUSkinVertexBuffer(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar, EUnrealEngineObjectUE4Version::STATIC_SKELETAL_MESH_SERIALIZATION_FIX);
    uint32 NumTexCoords = 0;
    bool bUseFullPrecisionUVs = false;
    Ar << NumTexCoords << bUseFullPrecisionUVs;
    //! Before UE 4.15 the GPU skin vertices hold the bone influences.
    const bool bInlineInfluences = !HasSeparateSkinWeightBuffer(Ar);
    bool bExtraBoneInfluences = false;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::SUPPORT_GPUSKINNING_8_BONE_INFLUENCES && bInlineInfluences)
        Ar << bExtraBoneInfluences;
    FVector3f MeshExtension, MeshOrigin;
    Ar << MeshExtension << MeshOrigin;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped())
        return;
    if (NumTexCoords == 0 || NumTexCoords > 8) {
        Ar.SetError(ErrorCode::InvalidData, "FSkeletalMeshVertexBuffer.NumTexCoords");
        return;
    }
    const int32 NumInfluences = bInlineInfluences ? (bExtraBoneInfluences ? 8 : 4) : 0;
    const int32 UVSize = bUseFullPrecisionUVs ? 8 : 4;
    const int32 ElementSize = 4 + 4 + NumInfluences * 2 + 12 + static_cast<int32>(NumTexCoords) * UVSize;
    int32 SerializedElementSize = 0;
    Ar << SerializedElementSize;
    const int32 Count = Ar.ReadCount(ElementSize);
    if (Ar.IsError())
        return;
    if (Count > 0 && SerializedElementSize != ElementSize) {
        Ar.SetError(ErrorCode::InvalidData, "FSkeletalMeshVertexBuffer.ElementSize");
        return;
    }
    TArray<FVector3f> Positions;
    TArray<FMeshTangents> Tangents;
    TArray<FVector2f> TexCoords;
    TArray<uint16> BoneIndices;
    TArray<uint16> BoneWeights;
    Positions.Empty(Count);
    Tangents.Empty(Count);
    TexCoords.Empty(Count * static_cast<int32>(NumTexCoords));
    BoneIndices.Empty(Count * NumInfluences);
    BoneWeights.Empty(Count * NumInfluences);
    for (int32 Vertex = 0; Vertex < Count && !Ar.IsError(); Vertex++) {
        FPackedNormal TangentX, TangentZ;
        Ar << TangentX.Packed << TangentZ.Packed;
        Tangents.Add(FMeshTangents{TangentX.ToFVector4f(), TangentZ.ToFVector4f()});
        for (int32 Slot = 0; Slot < NumInfluences; Slot++) {
            uint8 Bone = 0;
            Ar << Bone;
            BoneIndices.Add(Bone);
        }
        for (int32 Slot = 0; Slot < NumInfluences; Slot++) {
            uint8 Weight = 0;
            Ar << Weight;
            BoneWeights.Add(Weight);
        }
        FVector3f Position;
        Ar << Position;
        Positions.Add(Position);
        for (uint32 Channel = 0; Channel < NumTexCoords; Channel++) {
            FVector2f UV;
            if (bUseFullPrecisionUVs) {
                Ar << UV.X << UV.Y;
            } else {
                uint16 U = 0, V = 0;
                Ar << U << V;
                UV = FVector2f(HalfToFloat(U), HalfToFloat(V));
            }
            TexCoords.Add(UV);
        }
    }
    StaticVertexBuffers.PositionVertexBuffer.Init(std::move(Positions));
    StaticVertexBuffers.StaticMeshVertexBuffer.Init(std::move(Tangents), std::move(TexCoords), NumTexCoords, bUseFullPrecisionUVs);
    if (bInlineInfluences)
        SkinWeightVertexBuffer.Init(std::move(BoneIndices), std::move(BoneWeights), static_cast<uint32>(NumInfluences), static_cast<uint32>(Count));
}

void FSkeletalMeshLODRenderData::SerializeStaticLODModel(FArchive& Ar, bool bHasVertexColors) {
    if (Ar.GetGame() == EGame::GAME_SeaOfThieves)
        Ar.SkipBytes(4);
    FStripDataFlags StripFlags(Ar);
    if (Ar.GetGame() == EGame::GAME_SeaOfThieves)
        Ar.SkipBytes(4);
    const bool bStripped = StripFlags.IsAudioVisualDataStripped();
    TArray<FSkelMeshRenderSection> DummySections;
    FMultiSizeIndexContainer DummyIndexBuffer;
    TArray<FBoneIndexType> DummyActiveBoneIndices;
    SerializeStaticLODSections(Ar, bStripped ? DummySections : RenderSections);
    (bStripped ? DummyIndexBuffer : MultiSizeIndexContainer).Serialize(Ar);
    Ar << (bStripped ? DummyActiveBoneIndices : ActiveBoneIndices);
    if (!HasCombinedSectionsAndChunks(Ar)) {
        TArray<FSkelMeshRenderSection>& Sections = bStripped ? DummySections : RenderSections;
        const int32 ChunkCount = Ar.ReadCount(4);
        if (!Ar.IsError() && ChunkCount != Sections.Num()) {
            Ar.SetError(ErrorCode::InvalidData, "FStaticLODModel.Chunks");
            return;
        }
        for (int32 i = 0; i < ChunkCount && !Ar.IsError(); i++)
            SerializeLegacyChunk(Ar, Sections[i]);
    }
    uint32 LegacySize = 0;
    Ar << LegacySize;
    if (!bStripped) {
        uint32 NumVertices = 0;
        Ar << NumVertices;
    }
    Ar << RequiredBones;
    if (!StripFlags.IsEditorDataStripped()) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FStaticLODModel.RawPointIndices");
        return;
    }
    if (Ar.GetGame() != EGame::GAME_StateOfDecay2) {
        SkipArray(Ar, 4);
        int32 MaxImportVertex = 0;
        Ar << MaxImportVertex;
    }
    if (bStripped || Ar.IsError()) {
        SkipSeaOfThievesIndexContainer(Ar);
        return;
    }
    uint32 NumTexCoords = 0;
    Ar << NumTexCoords;
    SerializeGPUSkinVertexBuffer(Ar);
    if (HasSeparateSkinWeightBuffer(Ar))
        SkinWeightVertexBuffer.Serialize(Ar);
    if (bHasVertexColors) {
        //! Before UE 4.15 skeletal meshes store their colors in a plain FColor array.
        if (Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::UseSharedColorBufferFormat, EGame::GAME_UE4_15)) {
            StaticVertexBuffers.ColorVertexBuffer.Serialize(Ar);
        } else {
            SerializeLegacyColorBuffer(Ar, StaticVertexBuffers.ColorVertexBuffer);
        }
    }
    if (Ar.GetGame() == EGame::GAME_SeaOfThieves) {
        const int32 Count = Ar.ReadCount(44);
        Ar.SkipBytes(static_cast<int64>(Count) * 44);
        for (int32 i = 0; i < 4; i++)
            SkipArray(Ar, 4);
        Ar.SkipBytes(13);
    }
    if (Ar.GetGame() == EGame::GAME_FinalFantasy7Remake) {
        SerializeFinalFantasy7RemakeBuffers(Ar, StaticVertexBuffers.ColorVertexBuffer);
        bHasBuffers = !Ar.IsError();
        return;
    }
    if (!StripFlags.IsClassDataStripped(CDSF_AdjacencyData) && Ar.GetGame() != EGame::GAME_StateOfDecay2) {
        FMultiSizeIndexContainer AdjacencyMultiSizeIndexContainer;
        AdjacencyMultiSizeIndexContainer.Serialize(Ar);
    }
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::APEX_CLOTH && HasClothData())
        SkipLegacyClothVertexBuffer(Ar);
    if (Ar.GetGame() == EGame::GAME_DaysGone) {
        FMultiSizeIndexContainer DaysGoneIndexContainer;
        DaysGoneIndexContainer.Serialize(Ar);
        Ar.SkipBytes(12);
    }
    if (Ar.GetGame() == EGame::GAME_StateOfDecay2)
        Ar.SkipBytes(8);
    SkipSeaOfThievesIndexContainer(Ar);
    bHasBuffers = !Ar.IsError();
}

void FSkeletalMeshLODRenderData::SerializeLegacy(FArchive& Ar, bool bHasVertexColors, uint8 NumVertexColorChannels) {
    FStripDataFlags StripFlags(Ar);
    const bool bStripped = StripFlags.IsAudioVisualDataStripped() || StripFlags.IsClassDataStripped(MinLodStripFlag);
    if (bStripped) {
        TArray<FSkelMeshRenderSection> DummySections;
        FMultiSizeIndexContainer DummyIndexBuffer;
        TArray<FBoneIndexType> DummyActiveBoneIndices;
        Ar << DummySections;
        DummyIndexBuffer.Serialize(Ar);
        Ar << DummyActiveBoneIndices;
    } else {
        Ar << RenderSections;
        MultiSizeIndexContainer.Serialize(Ar);
        Ar << ActiveBoneIndices;
    }
    Ar << RequiredBones;
    if (bStripped || Ar.IsError())
        return;
    StaticVertexBuffers.PositionVertexBuffer.Serialize(Ar);
    StaticVertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    SkinWeightVertexBuffer.Serialize(Ar);
    //! Borderlands 3 stores one color buffer per vertex color channel when the mesh has no vertex colors.
    if (!bHasVertexColors && Ar.GetGame() == EGame::GAME_Borderlands3) {
        for (uint8 i = 0; i < NumVertexColorChannels && !Ar.IsError(); i++)
            StaticVertexBuffers.ColorVertexBuffer.Serialize(Ar);
    } else if (bHasVertexColors) {
        StaticVertexBuffers.ColorVertexBuffer.Serialize(Ar);
    }
    if (!StripFlags.IsClassDataStripped(CDSF_AdjacencyData)) {
        FMultiSizeIndexContainer AdjacencyMultiSizeIndexContainer;
        AdjacencyMultiSizeIndexContainer.Serialize(Ar);
    }
    if (HasClothData())
        SkipClothVertexBuffer(Ar);
    //! UE 4.23 adds skin weight profiles to the LOD data.
    if (Ar.GetGame() >= EGame::GAME_UE4_23)
        SkipSkinWeightProfiles(Ar);
    bHasBuffers = !Ar.IsError();
}

void FSkeletalMeshLODRenderData::Serialize(FArchive& Ar, bool bHasVertexColors, uint8 NumVertexColorChannels) {
    //! UE 4.19 replaces FStaticLODModel with FSkeletalMeshLODRenderData.
    if (Ar.GetGame() < EGame::GAME_UE4_19) {
        SerializeStaticLODModel(Ar, bHasVertexColors);
        return;
    }
    //! UE 4.24 splits the LOD data into inline metadata and streamed buffers.
    if (Ar.GetGame() < EGame::GAME_UE4_24) {
        SerializeLegacy(Ar, bHasVertexColors, NumVertexColorChannels);
        return;
    }
    SerializeCooked(Ar, bHasVertexColors);
    switch (Ar.GetGame()) {
    case EGame::GAME_ReadyOrNot:
    case EGame::GAME_HellLetLoose:
    case EGame::GAME_DarkPicturesAnthologyManofMedan:
    case EGame::GAME_DarkPicturesAnthologyTheDevilinMe:
    case EGame::GAME_AliensFireteamElite:
        Ar.SkipBytes(4);
        return;
    case EGame::GAME_DarkPicturesAnthologyLittleHope:
        if (!bIsLODOptional)
            Ar.SkipBytes(4);
        return;
    default:
        return;
    }
}

void FSkeletalMeshLODRenderData::SerializeCooked(FArchive& Ar, bool bHasVertexColors) {
    const EGame Game = Ar.GetGame();
    FStripDataFlags StripFlags(Ar);
    if (Game != EGame::GAME_Splitgate)
        Ar << bIsLODOptional;
    Ar << bStreamedDataInlined << RequiredBones;
    if (StripFlags.IsAudioVisualDataStripped() || bIsLODOptional)
        return;
    Ar << RenderSections << ActiveBoneIndices;
    if (Game == EGame::GAME_KenaBridgeofSpirits)
        SkipArray(Ar, 1);
    if (Game == EGame::GAME_FragPunk)
        Ar.SkipBytes(4);
    uint32 BuffersSize = 0;
    Ar << BuffersSize;
    if (Ar.IsError())
        return;
    //! UE 5.8 cooks inlined LODs as bulk data with an inline payload.
    if (bStreamedDataInlined && Game < EGame::GAME_UE5_8) {
        SerializeStreamedData(Ar, bHasVertexColors);
        SkipInlinedGameData(Ar);
        return;
    }
    FByteBulkData BulkData;
    BulkData.Serialize(Ar);
    if (Ar.IsError() || BulkData.GetElementCount() == 0)
        return;
    Result<std::vector<std::byte>> Bytes = BulkData.Read();
    if (!Bytes) {
        Ar.SetError(ErrorCode::InvalidData, "FSkeletalMeshLODRenderData.BulkData");
        return;
    }
    if (Game == EGame::GAME_FinalFantasy7Rebirth) {
        SerializeFinalFantasy7Rebirth(Ar, *Bytes, bHasVertexColors);
        return;
    }
    FMemoryReaderView BufferAr(*Bytes);
    BufferAr.CopyStateFrom(Ar);
    SerializeStreamedData(BufferAr, bHasVertexColors);
    if (BufferAr.IsError()) {
        Ar.SetError(BufferAr.GetError().code, BufferAr.GetError().where);
        return;
    }
    SerializeAvailabilityInfo(Ar, StripFlags.IsClassDataStripped(CDSF_AdjacencyData));
}

namespace {

template <class T, class ReadType>
TArray<T> ReadAt(FArchive& Ar, int32 Offset, int32 Count, ReadType&& Read) {
    TArray<T> Values;
    Ar.Seek(Offset);
    if (Count < 0 || static_cast<int64>(Count) > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FSkeletalMeshLODRenderData.FinalFantasy7Rebirth");
        return Values;
    }
    Values.Reserve(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++)
        Values.Add(Read());
    return Values;
}

}

//! Final Fantasy VII Rebirth keeps the buffer layout of a LOD in 161 inline bytes and the buffers at those offsets in the bulk data.
void FSkeletalMeshLODRenderData::SerializeFinalFantasy7Rebirth(FArchive& Ar, std::span<const std::byte> Buffers, bool bHasVertexColors) {
    std::array<std::byte, 161> Layout{};
    Ar.Serialize(Layout.data(), static_cast<int64>(Layout.size()));
    FMemoryReaderView Meta(Layout);
    const uint8 IndexSize = Meta.Read<uint8>();
    const int32 NumIndices = Meta.Read<int32>();
    const int32 NumTexCoords = Meta.Read<int32>();
    const int32 NumVertices = Meta.Read<int32>();
    const bool bFullPrecisionUVs = Meta.Read<int32>() != 0;
    const bool bHighPrecisionTangents = Meta.Read<int32>() != 0;
    Meta.SkipBytes(12);
    const int32 NumColors = Meta.Read<int32>();
    Meta.SkipBytes(4);
    const int32 MaxBoneInfluences = Meta.Read<int32>();
    Meta.SkipBytes(4);
    const int32 NumWeights = Meta.Read<int32>();
    const bool b16BitBoneIndex = Meta.Read<int32>() != 0;
    Meta.SkipBytes(4);
    const int32 ProfileCount = Meta.ReadCount(0);
    for (int32 i = 0; i < ProfileCount && !Ar.IsError(); i++) {
        FName Profile;
        Ar << Profile;
    }
    const int32 PositionOffset = Meta.Read<int32>();
    Meta.SkipBytes(4);
    const int32 TangentOffset = Meta.Read<int32>();
    Meta.SkipBytes(4);
    const int32 UVOffset = Meta.Read<int32>();
    Meta.SkipBytes(12);
    const int32 WeightOffset = Meta.Read<int32>();
    Meta.SkipBytes(4);
    const int32 ColorOffset = Meta.Read<int32>();
    Meta.SkipBytes(28);
    const int32 IndexOffset = Meta.Read<int32>();
    if (Meta.IsError() || Ar.IsError()) {
        Ar.SetError(ErrorCode::InvalidData, "FSkeletalMeshLODRenderData.FinalFantasy7Rebirth");
        return;
    }

    FMemoryReaderView BufferAr(Buffers);
    BufferAr.CopyStateFrom(Ar);
    TArray<uint32> Indices = ReadAt<uint32>(BufferAr, IndexOffset, NumIndices,
                                            [&] { return IndexSize == 4 ? BufferAr.Read<uint32>() : static_cast<uint32>(BufferAr.Read<uint16>()); });
    TArray<FVector3f> Positions = ReadAt<FVector3f>(BufferAr, PositionOffset, NumVertices, [&] { return BufferAr.Read<FVector3f>(); });
    TArray<FMeshTangents> Tangents = ReadAt<FMeshTangents>(BufferAr, TangentOffset, NumVertices, [&] { return ReadTangents(BufferAr, bHighPrecisionTangents); });
    TArray<FVector2f> TexCoords = ReadAt<FVector2f>(BufferAr, UVOffset, NumVertices * NumTexCoords, [&] { return ReadUV(BufferAr, bFullPrecisionUVs); });
    const int32 Influences = MaxBoneInfluences > 4 ? 8 : 4;
    TArray<uint16> BoneIndices;
    TArray<uint16> BoneWeights;
    BufferAr.Seek(WeightOffset);
    for (int32 Vertex = 0; Vertex < NumWeights && !BufferAr.IsError(); Vertex++) {
        for (int32 i = 0; i < Influences; i++)
            BoneIndices.Add(b16BitBoneIndex ? BufferAr.Read<uint16>() : static_cast<uint16>(BufferAr.Read<uint8>()));
        for (int32 i = 0; i < Influences; i++)
            BoneWeights.Add(BufferAr.Read<uint8>());
    }
    TArray<FColor> Colors;
    if (bHasVertexColors && ColorOffset != -1)
        Colors = ReadAt<FColor>(BufferAr, ColorOffset, NumColors, [&] { return BufferAr.Read<FColor>(); });
    if (BufferAr.IsError() || NumWeights < NumVertices) {
        Ar.SetError(ErrorCode::InvalidData, "FSkeletalMeshLODRenderData.FinalFantasy7Rebirth");
        return;
    }
    BoneIndices.SetNum(NumVertices * Influences);
    BoneWeights.SetNum(NumVertices * Influences);
    MultiSizeIndexContainer.Init(std::move(Indices), IndexSize == 4 ? 4 : 2);
    StaticVertexBuffers.PositionVertexBuffer.Init(std::move(Positions));
    StaticVertexBuffers.StaticMeshVertexBuffer.Init(std::move(Tangents), std::move(TexCoords), static_cast<uint32>(NumTexCoords), true);
    SkinWeightVertexBuffer.Init(std::move(BoneIndices), std::move(BoneWeights), static_cast<uint32>(Influences), static_cast<uint32>(NumVertices));
    if (!Colors.IsEmpty())
        StaticVertexBuffers.ColorVertexBuffer.Init(std::move(Colors));
    bHasBuffers = true;
}

void FSkeletalMeshRenderData::Serialize(FArchive& Ar, bool bHasVertexColors, uint8 NumVertexColorChannels) {
    const int32 LODCount = Ar.ReadCount(16);
    LODRenderData.Empty(LODCount);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++)
        LODRenderData[LODRenderData.AddDefaulted()].Serialize(Ar, bHasVertexColors, NumVertexColorChannels);
    //! Stalker 2 adds fallback LODs after the LODs.
    if (Ar.GetGame() == EGame::GAME_Stalker2) {
        const int32 FallbackLODCount = Ar.ReadCount(16);
        for (int32 i = 0; i < FallbackLODCount && !Ar.IsError(); i++)
            LODRenderData[LODRenderData.AddDefaulted()].Serialize(Ar, bHasVertexColors, NumVertexColorChannels);
    }
    //! UE 5.5 adds Nanite resources to skeletal meshes.
    if (Ar.GetGame() >= EGame::GAME_UE5_5)
        FNaniteResources().Serialize(Ar);
    if (Ar.GetGame() == EGame::GAME_DeadzoneRogue)
        Ar.SkipBytes(4);
    //! UE 4.24 adds the inlined and optional LOD counts.
    if (Ar.GetGame() >= EGame::GAME_UE4_24)
        Ar << NumInlinedLODs << NumNonOptionalLODs;
}

//! Game for Peace keeps each LOD model in its own bulk data, or adds skin weight profile data after each LOD model.
void FSkeletalMeshRenderData::SerializeGameForPeace(FArchive& Ar, bool bHasVertexColors, bool bIsStreamable, bool bHasSkinWeightProfiles) {
    const int32 LODCount = Ar.ReadCount(4);
    LODRenderData.Empty(LODCount);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++) {
        FSkeletalMeshLODRenderData& LOD = LODRenderData[LODRenderData.AddDefaulted()];
        if (!bIsStreamable) {
            LOD.Serialize(Ar, bHasVertexColors);
            if (!bHasSkinWeightProfiles)
                continue;
            bool bHasOverrides = false;
            Ar << bHasOverrides;
            if (bHasOverrides) {
                SkipBulkSerializedArray(Ar);
                SkipArray(Ar, 5);
                SkipArray(Ar, 2);
                SkipArray(Ar, 8);
            }
            bool bHasWeights = false;
            Ar << bHasWeights;
            if (bHasWeights) {
                SkipBulkSerializedArray(Ar);
                SkipArray(Ar, 16);
                SkipArray(Ar, 10);
            }
            continue;
        }
        FByteBulkData BulkData;
        BulkData.Serialize(Ar);
        if (Ar.IsError() || BulkData.GetElementCount() <= 0)
            continue;
        Result<std::vector<std::byte>> Bytes = BulkData.Read();
        if (!Bytes) {
            ReportLoadProblem("skeletalmesh.lod_bulk_unreadable", FString(std::to_string(i)));
            continue;
        }
        FMemoryReaderView LODAr(*Bytes);
        LODAr.CopyStateFrom(Ar);
        LOD.Serialize(LODAr, bHasVertexColors);
        if (LODAr.IsError())
            ReportLoadProblem("skeletalmesh.lod_bulk_invalid", FString(std::to_string(i)));
    }
}

}
