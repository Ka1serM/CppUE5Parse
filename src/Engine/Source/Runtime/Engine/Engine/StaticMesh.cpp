#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"

#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavCollisionBase.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"

namespace ue5parse::runtime {

namespace {

using V4 = EUnrealEngineObjectUE4Version;

constexpr uint8 CDSF_AdjacencyData = 1;
constexpr uint8 CDSF_MinLodData = 2;
constexpr uint8 CDSF_ReversedIndexBuffer = 4;
constexpr uint8 CDSF_RayTracingResources = 8;
constexpr int32 RayTracingRawDataHeaderSize = 6 * 4;
constexpr int32 MaxStreamingTexCoords = 8;
constexpr int32 DistanceFieldMips = 3;

bool HasTessellation(const FArchive& Ar) {
    return !Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::RemovingTessellation, EGame::GAME_UE5_0);
}

void SkipArray(FArchive& Ar, int32 ElementSize) {
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(ElementSize)) * ElementSize);
}

void SkipCardRepresentation(FArchive& Ar) {
    FBox Bounds;
    Ar << Bounds;
    if (Ar.GetGame() < EGame::GAME_UE5_1)
        Ar.SkipBytes(4);
    if (Ar.GetGame() >= EGame::GAME_UE5_2) {
        bool bMostlyTwoSided = false;
        Ar << bMostlyTwoSided;
    }
    SkipArray(Ar, Ar.GetGame() < EGame::GAME_UE5_1 ? 62 : 61);
}

//! Before 5.4 the distance field volume stores its bounds and mip transforms as large world doubles.
void SkipDistanceField5(FArchive& Ar) {
    const bool bFloatBounds = Ar.GetGame() >= EGame::GAME_UE5_4;
    if (bFloatBounds) {
        FBox3f Bounds;
        Ar << Bounds;
    } else {
        FBox Bounds;
        Ar << Bounds;
    }
    bool bMostlyTwoSided = false;
    Ar << bMostlyTwoSided;
    const int64 MipSize = 12 + 4 + (bFloatBounds ? 12 + 12 + 8 : 24 + 24 + 16) + 4 + 4;
    Ar.SkipBytes(DistanceFieldMips * MipSize);
    SkipArray(Ar, 1);
    FByteBulkData Streamable;
    Streamable.Serialize(Ar);
}

void SkipDistanceField4(FArchive& Ar) {
    const bool bCompressed = Ar.GetGame() >= EGame::GAME_UE4_16;
    SkipArray(Ar, bCompressed ? 1 : 2);
    FBox3f Bounds;
    Ar.SkipBytes(12);
    Ar << Bounds;
    if (bCompressed)
        Ar.SkipBytes(8);
    bool bFlag = false;
    Ar << bFlag;
    if (Ar.UEVer() >= V4::RENAME_CROUCHMOVESCHARACTERDOWN)
        Ar << bFlag;
    if (Ar.UEVer() >= V4::DEPRECATE_UMG_STYLE_ASSETS)
        Ar << bFlag;
}

void SkipNaniteHierarchyNodes(FArchive& Ar) {
    constexpr int32 Fanout = 4;
    const EGame Game = Ar.GetGame();
    const int32 Misc2Size = Game >= EGame::GAME_UE5_8 ? 12 : Game >= EGame::GAME_UE5_7 ? 8 : 4;
    SkipArray(Ar, Fanout * (16 + 12 + 4 + 12 + 4 + Misc2Size));
}

void SkipStaticMeshRayTracingProxy(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    bool bUsingRenderingLODs = false;
    Ar << bUsingRenderingLODs;
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    const int32 LODCount = Ar.ReadCount(8);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++) {
        bool bOwnsBuffers = false;
        Ar << bOwnsBuffers;
        if (bOwnsBuffers) {
            TArray<FStaticMeshSection> Sections;
            Ar << Sections;
        }
        bool bOwnsRayTracingGeometry = false;
        Ar << bOwnsRayTracingGeometry;
        bool bBuffersInlined = false;
        if (Ar.GetGame() >= EGame::GAME_UE5_6)
            Ar << bBuffersInlined;
        if (bBuffersInlined) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "FStaticMeshRayTracingProxyLOD.Inlined");
            return;
        }
        FByteBulkData Streamable;
        Streamable.Serialize(Ar);
        if (Ar.GetGame() < EGame::GAME_UE5_6)
            continue;
        if (bOwnsBuffers)
            Ar.SkipBytes(4 + 8 + 16 + 8 + 8);
        if (bOwnsRayTracingGeometry)
            Ar.SkipBytes(8 + RayTracingRawDataHeaderSize);
    }
}

}

void SkipWeightedRandomSampler(FArchive& Ar) {
    SkipArray(Ar, 4);
    SkipArray(Ar, 4);
    Ar.SkipBytes(4);
}

void SkipBulkSerializedArray(FArchive& Ar) {
    int32 ElementSize = 0, Count = 0;
    Ar << ElementSize << Count;
    if (ElementSize < 0 || Count < 0) {
        Ar.SetError(ErrorCode::InvalidData, "SkipBulkSerializedArray");
        return;
    }
    Ar.SkipBytes(static_cast<int64>(ElementSize) * Count);
}

void FNaniteResources::Serialize(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    const EGame Game = Ar.GetGame();
    Ar.SkipBytes(4);
    FByteBulkData StreamablePages;
    StreamablePages.Serialize(Ar);
    RootDataSize = Ar.ReadCount(1);
    Ar.SkipBytes(RootDataSize);
    NumPageStreamingStates = Ar.ReadCount(20);
    Ar.SkipBytes(static_cast<int64>(NumPageStreamingStates) * (Game >= EGame::GAME_UE5_3 ? 20 : 24));
    SkipNaniteHierarchyNodes(Ar);
    SkipArray(Ar, 4);
    SkipArray(Ar, Game >= EGame::GAME_UE5_7 ? 2 : 4);
    if (Game >= EGame::GAME_UE5_6) {
        SkipArray(Ar, 48);
        if (Game >= EGame::GAME_UE5_7) {
            SkipArray(Ar, 4);
            if (Game >= EGame::GAME_UE5_8)
                SkipArray(Ar, 4);
            SkipArray(Ar, 4);
        }
        Ar.SkipBytes(12 + 12 + 4);
    }
    if (Game < EGame::GAME_UE5_8)
        SkipArray(Ar, 2);
    Ar.SkipBytes(4 + 4);
    if (Game >= EGame::GAME_UE5_2)
        Ar.SkipBytes(4);
    Ar << NumInputTriangles << NumInputVertices;
    if (Game < EGame::GAME_UE5_6)
        Ar.SkipBytes(2 + 2);
    if (Game >= EGame::GAME_UE5_1)
        Ar.SkipBytes(4);
    if (Game >= EGame::GAME_UE5_7)
        Ar.SkipBytes(8);
    if (Game >= EGame::GAME_UE5_8)
        Ar.SkipBytes(4);
}

FArchive& operator<<(FArchive& Ar, FStaticMeshSection& Section) {
    Ar << Section.MaterialIndex << Section.FirstIndex << Section.NumTriangles << Section.MinVertexIndex << Section.MaxVertexIndex;
    Ar << Section.bEnableCollision << Section.bCastShadow;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::StaticMeshSectionForceOpaqueField, EGame::GAME_UE4_25))
        Ar << Section.bForceOpaque;
    if (Ar.GetGame() >= EGame::GAME_UE4_26)
        Ar << Section.bVisibleInRayTracing;
    if (Ar.GetGame() >= EGame::GAME_UE5_1)
        Ar << Section.bAffectDistanceFieldLighting;
    return Ar;
}

void FStaticMeshLODResources::SerializeBuffers(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    VertexBuffers.PositionVertexBuffer.Serialize(Ar);
    VertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    VertexBuffers.ColorVertexBuffer.Serialize(Ar);
    IndexBuffer.Serialize(Ar);
    FRawStaticIndexBuffer Unused;
    if (!StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer))
        Unused.Serialize(Ar);
    Unused.Serialize(Ar);
    if (!StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer))
        Unused.Serialize(Ar);
    if (!StripFlags.IsEditorDataStripped())
        Unused.Serialize(Ar);
    if (HasTessellation(Ar) && !StripFlags.IsClassDataStripped(CDSF_AdjacencyData))
        Unused.Serialize(Ar);
    if (Ar.GetGame() >= EGame::GAME_UE4_25 && !StripFlags.IsClassDataStripped(CDSF_RayTracingResources)) {
        if (Ar.GetGame() >= EGame::GAME_UE5_6)
            Ar.SkipBytes(RayTracingRawDataHeaderSize);
        SkipBulkSerializedArray(Ar);
    }
    for (int32 i = 0; i < Sections.Num(); i++)
        SkipWeightedRandomSampler(Ar);
    SkipWeightedRandomSampler(Ar);
    bHasBuffers = !Ar.IsError();
}

void FStaticMeshLODResources::SerializeLegacyBuffers(FArchive& Ar, const FStripDataFlags& StripFlags) {
    VertexBuffers.PositionVertexBuffer.Serialize(Ar);
    VertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    VertexBuffers.ColorVertexBuffer.Serialize(Ar);
    IndexBuffer.Serialize(Ar);
    FRawStaticIndexBuffer Unused;
    const bool bReversed = Ar.UEVer() >= V4::SOUND_CONCURRENCY_PACKAGE && !StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer);
    if (bReversed)
        Unused.Serialize(Ar);
    Unused.Serialize(Ar);
    if (bReversed)
        Unused.Serialize(Ar);
    if (Ar.UEVer() >= V4::FTEXT_HISTORY && Ar.UEVer() < V4::RENAME_CROUCHMOVESCHARACTERDOWN)
        SkipDistanceField4(Ar);
    if (!StripFlags.IsEditorDataStripped())
        Unused.Serialize(Ar);
    if (!StripFlags.IsClassDataStripped(CDSF_AdjacencyData))
        Unused.Serialize(Ar);
    if (Ar.GetGame() >= EGame::GAME_UE4_16) {
        for (int32 i = 0; i < Sections.Num(); i++)
            SkipWeightedRandomSampler(Ar);
        SkipWeightedRandomSampler(Ar);
    }
    bHasBuffers = !Ar.IsError();
}

void FStaticMeshLODResources::Serialize(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    Ar << Sections;
    if (Ar.GetGame() >= EGame::GAME_UE5_6)
        Ar << SourceMeshBounds;
    Ar << MaxDeviation;
    if (Ar.IsError())
        return;
    if (Ar.GetGame() < EGame::GAME_UE4_23) {
        if (!StripFlags.IsAudioVisualDataStripped() && !StripFlags.IsClassDataStripped(CDSF_MinLodData))
            SerializeLegacyBuffers(Ar, StripFlags);
        return;
    }
    Ar << bIsLODCookedOut << bBuffersInlined;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped() || bIsLODCookedOut)
        return;
    if (Ar.GetGame() >= EGame::GAME_UE5_5) {
        bool bHasRayTracingGeometry = false;
        Ar << bHasRayTracingGeometry;
    }
    if (bBuffersInlined) {
        SerializeBuffers(Ar);
    } else {
        FByteBulkData BulkData;
        BulkData.Serialize(Ar);
        if (BulkData.GetElementCount() > 0) {
            Result<std::vector<std::byte>> Bytes = BulkData.Read();
            if (Bytes) {
                FMemoryReaderView BufferAr(*Bytes);
                BufferAr.CopyStateFrom(Ar);
                SerializeBuffers(BufferAr);
                if (BufferAr.IsError())
                    bHasBuffers = false;
            }
        }
        Ar.SkipBytes(8 + 4 * 4 + 2 * 4 + 2 * 4 + 5 * 2 * 4);
        if (HasTessellation(Ar))
            Ar.SkipBytes(2 * 4);
        if (Ar.GetGame() >= EGame::GAME_UE5_6)
            Ar.SkipBytes(RayTracingRawDataHeaderSize);
    }
    Ar.SkipBytes(12);
}

void FStaticMeshRenderData::SerializeDistanceFields(FArchive& Ar) {
    if (Ar.UEVer() < V4::RENAME_CROUCHMOVESCHARACTERDOWN)
        return;
    if (Ar.UEVer() >= V4::RENAME_WIDGET_VISIBILITY) {
        FStripDataFlags StripFlags(Ar);
        if (StripFlags.IsAudioVisualDataStripped())
            return;
        if (Ar.GetGame() >= EGame::GAME_UE4_21 && StripFlags.IsClassDataStripped(1))
            return;
    }
    for (int32 i = 0; i < LODResources.Num() && !Ar.IsError(); i++) {
        bool bValid = false;
        Ar << bValid;
        if (!bValid)
            continue;
        if (Ar.GetGame() >= EGame::GAME_UE5_0)
            SkipDistanceField5(Ar);
        else
            SkipDistanceField4(Ar);
    }
}

void FStaticMeshRenderData::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    const int32 LODCount = Ar.ReadCount(16);
    LODResources.Empty(LODCount);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++)
        LODResources[LODResources.AddDefaulted()].Serialize(Ar);
    if (Game >= EGame::GAME_UE4_23)
        Ar << NumInlinedLODs;
    if (Game >= EGame::GAME_UE5_0) {
        NaniteResources.Serialize(Ar);
        if (Game >= EGame::GAME_UE5_5) {
            bool bHasRayTracingProxy = false;
            Ar << bHasRayTracingProxy;
            if (bHasRayTracingProxy)
                SkipStaticMeshRayTracingProxy(Ar);
        }
        FStripDataFlags CardFlags(Ar);
        if (!CardFlags.IsAudioVisualDataStripped() && !CardFlags.IsClassDataStripped(2))
            for (int32 i = 0; i < LODResources.Num() && !Ar.IsError(); i++) {
                bool bValid = false;
                Ar << bValid;
                if (bValid)
                    SkipCardRepresentation(Ar);
            }
    }
    SerializeDistanceFields(Ar);
    Ar << Bounds;
    //! UE 4.15 does not write bLODsShareStaticLighting.
    if (Game < EGame::GAME_UE4_15 || Game >= EGame::GAME_UE4_16) {
        if (Game >= EGame::GAME_UE5_6) {
            uint8 RenderDataFlags = 0;
            Ar << RenderDataFlags;
            bLODsShareStaticLighting = (RenderDataFlags & 1) != 0;
        } else {
            Ar << bLODsShareStaticLighting;
        }
    }
    if (Game < EGame::GAME_UE4_14) {
        bool bReducedBySimplygon = false;
        Ar << bReducedBySimplygon;
    }
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::TextureStreamingMeshUVChannelData, EGame::GAME_UE4_14))
        Ar.SkipBytes(4 * MaxStreamingTexCoords + 4);
    const int32 ScreenSizeCount = Game >= EGame::GAME_UE4_9 ? 8 : 4;
    for (int32 i = 0; i < ScreenSizeCount; i++) {
        if (Game >= EGame::GAME_UE4_20) {
            bool bCooked = false;
            Ar << bCooked;
        }
        Ar << ScreenSize[i];
    }
    if (Game >= EGame::GAME_UE5_4)
        FStripDataFlags RayTracingFlags(Ar);
}

FArchive& operator<<(FArchive& Ar, FStaticMaterial& Material) {
    UObject* Interface = nullptr;
    Ar << Interface << Material.MaterialSlotName;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::TextureStreamingMeshUVChannelData, EGame::GAME_UE4_14))
        Ar << Material.UVChannelData;
    Material.MaterialInterface = Cast<UMaterialInterface>(Interface);
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::MeshMaterialSlotOverlayMaterialAdded, EGame::GAME_UE5_6))
        return Ar;
    UObject* Overlay = nullptr;
    Ar << Overlay;
    Material.OverlayMaterialInterface = Cast<UMaterialInterface>(Overlay);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FMeshUVChannelInfo& Info) {
    Ar << Info.bInitialized << Info.bOverrideDensities;
    for (float& Density : Info.LocalUVDensities)
        Ar << Density;
    return Ar;
}

void UStaticMesh::SkipOccluderData(FArchive& Ar) {
    bool bHasOccluderData = false;
    Ar << bHasOccluderData;
    if (!bHasOccluderData)
        return;
    SkipArray(Ar, 12);
    SkipArray(Ar, 2);
}

void UStaticMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    const EGame Game = Ar.GetGame();
    FStripDataFlags StripFlags(Ar);
    bool bCooked = false;
    Ar << bCooked;
    UObject* BodySetupObject = nullptr;
    Ar << BodySetupObject;
    BodySetup = reinterpret_cast<UBodySetup*>(BodySetupObject);
    if (Ar.UEVer() >= V4::STATIC_MESH_STORE_NAV_COLLISION) {
        UObject* NavCollisionObject = nullptr;
        Ar << NavCollisionObject;
        NavCollision = Cast<UNavCollisionBase>(NavCollisionObject);
    }
    if (!StripFlags.IsEditorDataStripped()) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "UStaticMesh.EditorData");
        return;
    }
    Ar << LightingGuid;
    TArray<UObject*> SocketObjects;
    Ar << SocketObjects;
    Sockets.Empty(SocketObjects.Num());
    for (UObject* Socket : SocketObjects)
        Sockets.Add(Cast<UStaticMeshSocket>(Socket));
    if (bCooked) {
        RenderData = std::make_unique<FStaticMeshRenderData>();
        RenderData->Serialize(Ar);
        if (Ar.IsError())
            return;
        if (Game >= EGame::GAME_UE4_20 && Game < EGame::GAME_UE5_0)
            SkipOccluderData(Ar);
    }
    if (Game < EGame::GAME_UE4_14) {
        ConvertLegacyMaterials();
        return;
    }
    Ar << bHasSpeedTreeWind;
    if (bHasSpeedTreeWind) {
        Ar.Seek(Ar.TotalSize());
        return;
    }
    if (Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::RefactorMeshEditorMaterials, EGame::GAME_UE4_14))
        Ar << StaticMaterials;
}

void UStaticMesh::ConvertLegacyMaterials() {
    for (UMaterialInterface* Material : Materials) {
        FStaticMaterial& StaticMaterial = StaticMaterials.Emplace_GetRef();
        StaticMaterial.MaterialInterface = Material;
        StaticMaterial.MaterialSlotName = Material != nullptr ? Material->GetFName() : NAME_None;
    }
    Materials.Empty();
}

void UStaticMesh::PostLoad() {
    Super::PostLoad();
    //! Before UE 4.9 a static mesh has no ExtendedBounds property and returns the bounds of its render data.
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE4_9 && RenderData != nullptr)
        ExtendedBounds = RenderData->Bounds;
}

int32 UStaticMesh::GetNumNaniteVertices() const {
    if (RenderData == nullptr || !RenderData->HasValidNaniteData() || RenderData->NaniteResources.RootDataSize == 0)
        return 0;
    return static_cast<int32>(RenderData->NaniteResources.NumInputVertices);
}

int32 UStaticMesh::GetNumNaniteTriangles() const {
    if (RenderData == nullptr || !RenderData->HasValidNaniteData() || RenderData->NaniteResources.RootDataSize == 0)
        return 0;
    return static_cast<int32>(RenderData->NaniteResources.NumInputTriangles);
}

UMaterialInterface* UStaticMesh::GetMaterial(int32 MaterialIndex) const {
    return StaticMaterials.IsValidIndex(MaterialIndex) ? StaticMaterials[MaterialIndex].MaterialInterface : nullptr;
}

UStaticMeshSocket* UStaticMesh::FindSocket(FName InSocketName) const {
    if (InSocketName.isNone())
        return nullptr;
    for (UStaticMeshSocket* Socket : Sockets)
        if (Socket != nullptr && Socket->SocketName == InSocketName)
            return Socket;
    return nullptr;
}

int32 UStaticMesh::GetMaterialIndex(FName MaterialSlotName) const {
    for (int32 Index = 0; Index < StaticMaterials.Num(); Index++)
        if (StaticMaterials[Index].MaterialSlotName == MaterialSlotName)
            return Index;
    return INDEX_NONE;
}

}
