#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"

#include <vector>

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"

namespace ue5parse::runtime {

namespace {

constexpr uint8 CDSF_ReversedIndexBuffer = 4;
constexpr uint8 CDSF_RayTracingResources = 8;

void SkipCardRepresentation(FArchive& Ar) {
    Ar.SkipBytes(24 + 24 + 1);
    bool bMostlyTwoSided = false;
    Ar << bMostlyTwoSided;
    const int32 Count = Ar.ReadCount(61);
    Ar.SkipBytes(static_cast<int64>(Count) * 61);
}

void SkipDistanceField(FArchive& Ar) {
    Ar.SkipBytes(24 + 24 + 1);
    bool bMostlyTwoSided = false;
    Ar << bMostlyTwoSided;
    for (int32 Mip = 0; Mip < 3; Mip++) Ar.SkipBytes(12 + 4 + 24 + 24 + 16 + 8);
    Ar.SkipBytes(Ar.ReadCount(1));
    FByteBulkData Streamable;
    Streamable.Serialize(Ar);
}

}

void SkipWeightedRandomSampler(FArchive& Ar) {
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
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

bool SerializeNaniteResources(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped()) return false;
    Ar.SkipBytes(4);
    FByteBulkData StreamablePages;
    StreamablePages.Serialize(Ar);
    Ar.SkipBytes(Ar.ReadCount(1));
    const int32 PageCount = Ar.ReadCount(20);
    Ar.SkipBytes(static_cast<int64>(PageCount) * 20);
    const int32 NodeCount = Ar.ReadCount(48 * 4);
    Ar.SkipBytes(static_cast<int64>(NodeCount) * 48 * 4);
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(2)) * 2);
    Ar.SkipBytes(4 + 4 + 4);
    Ar.SkipBytes(4 + 4);
    Ar.SkipBytes(2 + 2);
    Ar.SkipBytes(4);
    return PageCount > 0;
}

FArchive& operator<<(FArchive& Ar, FStaticMeshSection& Section) {
    Ar << Section.MaterialIndex << Section.FirstIndex << Section.NumTriangles << Section.MinVertexIndex << Section.MaxVertexIndex;
    Ar << Section.bEnableCollision << Section.bCastShadow << Section.bForceOpaque << Section.bVisibleInRayTracing;
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
    if (!StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer)) Unused.Serialize(Ar);
    Unused.Serialize(Ar);
    if (!StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer)) Unused.Serialize(Ar);
    if (!StripFlags.IsEditorDataStripped()) Unused.Serialize(Ar);
    if (!StripFlags.IsClassDataStripped(CDSF_RayTracingResources)) SkipBulkSerializedArray(Ar);
    for (int32 i = 0; i < Sections.Num(); i++) SkipWeightedRandomSampler(Ar);
    SkipWeightedRandomSampler(Ar);
    bHasBuffers = !Ar.IsError();
}

void FStaticMeshLODResources::Serialize(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    const int32 SectionCount = Ar.ReadCount(4 * 5 + 4 * 5);
    Sections.Empty(SectionCount);
    for (int32 i = 0; i < SectionCount && !Ar.IsError(); i++) Ar << Sections[Sections.AddDefaulted()];
    Ar << MaxDeviation << bIsLODCookedOut << bBuffersInlined;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped() || bIsLODCookedOut) return;
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
                if (BufferAr.IsError()) bHasBuffers = false;
            }
        }
        Ar.SkipBytes(8 + 4 * 4 + 2 * 4 + 2 * 4 + 5 * 2 * 4);
    }
    Ar.SkipBytes(12);
}

void FStaticMeshRenderData::Serialize(FArchive& Ar) {
    const int32 LODCount = Ar.ReadCount(16);
    LODResources.Empty(LODCount);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++) LODResources[LODResources.AddDefaulted()].Serialize(Ar);
    Ar << NumInlinedLODs;
    bHasNaniteData = SerializeNaniteResources(Ar);
    FStripDataFlags CardFlags(Ar);
    if (!CardFlags.IsAudioVisualDataStripped() && !CardFlags.IsClassDataStripped(2))
        for (int32 i = 0; i < LODResources.Num(); i++) {
            bool bValid = false;
            Ar << bValid;
            if (bValid) SkipCardRepresentation(Ar);
        }
    FStripDataFlags FieldFlags(Ar);
    if (!FieldFlags.IsAudioVisualDataStripped() && !FieldFlags.IsClassDataStripped(1))
        for (int32 i = 0; i < LODResources.Num(); i++) {
            bool bValid = false;
            Ar << bValid;
            if (bValid) SkipDistanceField(Ar);
        }
    Ar << Bounds << bLODsShareStaticLighting;
    for (float& LODScreenSize : ScreenSize) {
        bool bCooked = false;
        Ar << bCooked << LODScreenSize;
    }
}

FArchive& operator<<(FArchive& Ar, FStaticMaterial& Material) {
    UObject* Interface = nullptr;
    Ar << Interface << Material.MaterialSlotName << Material.UVChannelData;
    Material.MaterialInterface = Cast<UMaterialInterface>(Interface);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FMeshUVChannelInfo& Info) {
    Ar << Info.bInitialized << Info.bOverrideDensities;
    for (float& Density : Info.LocalUVDensities) Ar << Density;
    return Ar;
}

void UStaticMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    FStripDataFlags StripFlags(Ar);
    bool bCooked = false;
    Ar << bCooked;
    UObject* BodySetupObject = nullptr;
    UObject* NavCollisionObject = nullptr;
    Ar << BodySetupObject << NavCollisionObject;
    BodySetup = reinterpret_cast<UBodySetup*>(BodySetupObject);
    NavCollision = NavCollisionObject;
    if (!StripFlags.IsEditorDataStripped()) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "UStaticMesh.EditorData");
        return;
    }
    Ar << LightingGuid;
    TArray<UObject*> SocketObjects;
    Ar << SocketObjects;
    Sockets.Empty(SocketObjects.Num());
    for (UObject* Socket : SocketObjects) Sockets.Add(Cast<UStaticMeshSocket>(Socket));
    if (bCooked) {
        RenderData = std::make_unique<FStaticMeshRenderData>();
        RenderData->Serialize(Ar);
        if (Ar.IsError()) return;
    }
    Ar << bHasSpeedTreeWind;
    if (bHasSpeedTreeWind) {
        Ar.Seek(Ar.TotalSize());
        return;
    }
    Ar << StaticMaterials;
}

void UStaticMesh::PostLoad() {
    Super::PostLoad();
    CalculateExtendedBounds();
}

void UStaticMesh::CalculateExtendedBounds() {
    FBoxSphereBounds Bounds;
    if (RenderData != nullptr) Bounds = RenderData->Bounds;
    if (!NegativeBoundsExtension.IsZero() || !PositiveBoundsExtension.IsZero()) {
        const FVector Min = Bounds.Origin - Bounds.BoxExtent - NegativeBoundsExtension;
        const FVector Max = Bounds.Origin + Bounds.BoxExtent + PositiveBoundsExtension;
        Bounds.Origin = (Min + Max) / 2.0;
        Bounds.BoxExtent = (Max - Min) / 2.0;
        Bounds.SphereRadius = Bounds.BoxExtent.Size();
    }
    ExtendedBounds = Bounds;
}

UMaterialInterface* UStaticMesh::GetMaterial(int32 MaterialIndex) const {
    return StaticMaterials.IsValidIndex(MaterialIndex) ? StaticMaterials[MaterialIndex].MaterialInterface : nullptr;
}

UStaticMeshSocket* UStaticMesh::FindSocket(FName InSocketName) const {
    if (InSocketName.isNone()) return nullptr;
    for (UStaticMeshSocket* Socket : Sockets)
        if (Socket != nullptr && Socket->SocketName == InSocketName) return Socket;
    return nullptr;
}

int32 UStaticMesh::GetMaterialIndex(FName MaterialSlotName) const {
    for (int32 Index = 0; Index < StaticMaterials.Num(); Index++)
        if (StaticMaterials[Index].MaterialSlotName == MaterialSlotName) return Index;
    return INDEX_NONE;
}

}
