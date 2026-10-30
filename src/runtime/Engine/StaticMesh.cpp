#include "ue5parse/runtime/Engine/Engine/StaticMesh.h"

#include <vector>

#include "ue5parse/runtime/CoreUObject/Serialization/BulkData.h"
#include "ue5parse/runtime/Engine/EngineUtils.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInterface.h"

// UE's StaticMesh.cpp and the cooked render data of StaticMeshRender/StaticMeshResources (UE 5.3).

namespace ue5parse::runtime {

namespace {

// EClassDataStripFlag of FStaticMeshLODResources.
constexpr uint8 CDSF_ReversedIndexBuffer = 4;
constexpr uint8 CDSF_RayTracingResources = 8;

// FCardRepresentationData (Lumen cards).
void SkipCardRepresentation(FArchive& Ar) {
    Ar.SkipBytes(24 + 24 + 1);   // Bounds (FBox)
    bool bMostlyTwoSided = false;
    Ar << bMostlyTwoSided;
    const int32 Count = Ar.ReadCount(61);
    // FLumenCardBuildData: an OBB of five FVector3f and the axis direction.
    Ar.SkipBytes(static_cast<int64>(Count) * 61);
}

// FDistanceFieldVolumeData: bounds, three sparse mips, the always loaded mip and the streamed ones.
void SkipDistanceField(FArchive& Ar) {
    Ar.SkipBytes(24 + 24 + 1);
    bool bMostlyTwoSided = false;
    Ar << bMostlyTwoSided;
    for (int32 Mip = 0; Mip < 3; Mip++) Ar.SkipBytes(12 + 4 + 24 + 24 + 16 + 8);
    Ar.SkipBytes(Ar.ReadCount(1));   // AlwaysLoadedMip
    FByteBulkData Streamable;
    Streamable.Serialize(Ar);
}

}

void SkipWeightedRandomSampler(FArchive& Ar) {
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);   // Prob
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);   // Alias
    Ar.SkipBytes(4);                                         // TotalWeight
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
    Ar.SkipBytes(4);   // ResourceFlags
    FByteBulkData StreamablePages;
    StreamablePages.Serialize(Ar);
    Ar.SkipBytes(Ar.ReadCount(1));   // RootData
    const int32 PageCount = Ar.ReadCount(20);
    Ar.SkipBytes(static_cast<int64>(PageCount) * 20);   // PageStreamingStates
    const int32 NodeCount = Ar.ReadCount(48 * 4);
    Ar.SkipBytes(static_cast<int64>(NodeCount) * 48 * 4);   // HierarchyNodes
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);   // HierarchyRootOffsets
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);   // PageDependencies
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(2)) * 2);   // ImposterAtlas
    Ar.SkipBytes(4 + 4 + 4);   // NumRootPages, PositionPrecision, NormalPrecision
    Ar.SkipBytes(4 + 4);       // NumInputTriangles, NumInputVertices
    Ar.SkipBytes(2 + 2);       // NumInputMeshes, NumInputTexCoords
    Ar.SkipBytes(4);           // NumClusters
    return PageCount > 0;
}

FArchive& operator<<(FArchive& Ar, FStaticMeshSection& Section) {
    Ar << Section.MaterialIndex << Section.FirstIndex << Section.NumTriangles << Section.MinVertexIndex << Section.MaxVertexIndex;
    Ar << Section.bEnableCollision << Section.bCastShadow << Section.bForceOpaque << Section.bVisibleInRayTracing;
    Ar << Section.bAffectDistanceFieldLighting;
    return Ar;
}

int32 FStaticMeshLODResources::GetNumTriangles() const {
    int32 Count = 0;
    for (const FStaticMeshSection& Section : Sections) Count += static_cast<int32>(Section.NumTriangles);
    return Count;
}

void FStaticMeshLODResources::SerializeBuffers(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    VertexBuffers.PositionVertexBuffer.Serialize(Ar);
    VertexBuffers.StaticMeshVertexBuffer.Serialize(Ar);
    VertexBuffers.ColorVertexBuffer.Serialize(Ar);
    IndexBuffer.Serialize(Ar);
    FRawStaticIndexBuffer Unused;
    if (!StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer)) Unused.Serialize(Ar);   // ReversedIndexBuffer
    Unused.Serialize(Ar);                                                                     // DepthOnlyIndexBuffer
    if (!StripFlags.IsClassDataStripped(CDSF_ReversedIndexBuffer)) Unused.Serialize(Ar);   // ReversedDepthOnlyIndexBuffer
    if (!StripFlags.IsEditorDataStripped()) Unused.Serialize(Ar);                           // WireframeIndexBuffer
    if (!StripFlags.IsClassDataStripped(CDSF_RayTracingResources)) SkipBulkSerializedArray(Ar);   // RayTracingGeometry
    for (int32 i = 0; i < Sections.Num(); i++) SkipWeightedRandomSampler(Ar);   // AreaWeightedSectionSamplers
    SkipWeightedRandomSampler(Ar);                                             // AreaWeightedSampler
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
        // A streamed LOD: the buffers are in bulk data.
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
        // DepthOnlyNumTriangles, Packed, and the sizes of the buffers.
        Ar.SkipBytes(8 + 4 * 4 + 2 * 4 + 2 * 4 + 5 * 2 * 4);
    }
    Ar.SkipBytes(12);   // FStaticMeshBuffersSize
}

void FStaticMeshRenderData::Serialize(FArchive& Ar) {
    const int32 LODCount = Ar.ReadCount(16);
    LODResources.Empty(LODCount);
    for (int32 i = 0; i < LODCount && !Ar.IsError(); i++) LODResources[LODResources.AddDefaulted()].Serialize(Ar);
    Ar << NumInlinedLODs;
    bHasNaniteData = SerializeNaniteResources(Ar);
    // Lumen cards of each LOD.
    FStripDataFlags CardFlags(Ar);
    if (!CardFlags.IsAudioVisualDataStripped() && !CardFlags.IsClassDataStripped(2))
        for (int32 i = 0; i < LODResources.Num(); i++) {
            bool bValid = false;
            Ar << bValid;
            if (bValid) SkipCardRepresentation(Ar);
        }
    // Distance fields of each LOD.
    FStripDataFlags FieldFlags(Ar);
    if (!FieldFlags.IsAudioVisualDataStripped() && !FieldFlags.IsClassDataStripped(1))
        for (int32 i = 0; i < LODResources.Num(); i++) {
            bool bValid = false;
            Ar << bValid;
            if (bValid) SkipDistanceField(Ar);
        }
    Ar << Bounds << bLODsShareStaticLighting;
    for (float& LODScreenSize : ScreenSize) {
        bool bCooked = false;   // FPerPlatformFloat: bCooked, then the default
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
        // FSpeedTreeWind is not read: the rest of the export is its data, and the materials stay as the properties
        // have them.
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
    // The extensions only when set: they make the sphere larger than the render data's.
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

int32 UStaticMesh::GetNumSections(int32 InLOD) const {
    return RenderData && RenderData->LODResources.IsValidIndex(InLOD) ? RenderData->LODResources[InLOD].Sections.Num() : 0;
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
