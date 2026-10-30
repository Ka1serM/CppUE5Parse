#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"

#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/EngineUtils.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInterface.h"

// UE's SkeletalMesh.cpp and SkinnedAsset.cpp (UE 5.3): the cooked skeletal mesh.

namespace ue5parse::runtime {

namespace {

const FReferenceSkeleton& GetEmptyRefSkeleton() {
    static const FReferenceSkeleton Empty;
    return Empty;
}

}

const FReferenceSkeleton& USkinnedAsset::GetRefSkeleton() const { return GetEmptyRefSkeleton(); }

const TArray<FSkeletalMaterial>& USkinnedAsset::GetMaterials() const {
    static const TArray<FSkeletalMaterial> Empty;
    return Empty;
}

FArchive& operator<<(FArchive& Ar, FSkeletalMaterial& Material) {
    UObject* Interface = nullptr;
    Ar << Interface << Material.MaterialSlotName;
    Material.MaterialInterface = Cast<UMaterialInterface>(Interface);
    bool bSerializeImportedMaterialSlotName = false;
    Ar << bSerializeImportedMaterialSlotName;
    if (bSerializeImportedMaterialSlotName) Ar << Material.ImportedMaterialSlotName;
    Ar << Material.UVChannelData;
    return Ar;
}

void USkeletalMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    FStripDataFlags StripFlags(Ar);
    Ar << ImportedBounds << Materials << RefSkeleton;
    if (Ar.IsError()) return;
    RefSkeleton.RebuildRefSkeleton(Skeleton, false);
    if (!StripFlags.IsEditorDataStripped()) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "USkeletalMesh.EditorData");
        return;
    }
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked && !IsTemplate()) {
        RenderData = std::make_unique<FSkeletalMeshRenderData>();
        RenderData->Serialize(Ar, bHasVertexColors);
        if (Ar.IsError()) return;
    }
    TArray<UObject*> DummyObjs;
    Ar << DummyObjs;
}

void USkeletalMesh::PostLoad() {
    Super::PostLoad();
    // The skeleton loads after the mesh's data was read: the virtual bones need it.
    RefSkeleton.RebuildRefSkeleton(Skeleton, false);
    CalculateInvRefMatrices();
    CalculateExtendedBounds();
    // A LOD that remaps its materials (LODInfo[i].LODMaterialMap) changes the material of its sections.
    if (RenderData == nullptr) return;
    for (int32 LODIndex = 0; LODIndex < RenderData->LODRenderData.Num(); LODIndex++) {
        TArray<int32> MaterialMap = GetLODMaterialMap(LODIndex);
        TArray<FSkelMeshRenderSection>& Sections = RenderData->LODRenderData[LODIndex].RenderSections;
        for (int32 SectionIndex = 0; SectionIndex < Sections.Num() && SectionIndex < MaterialMap.Num(); SectionIndex++) {
            const int32 Mapped = MaterialMap[SectionIndex];
            if (Materials.IsValidIndex(Mapped)) Sections[SectionIndex].MaterialIndex = static_cast<uint16>(Mapped);
        }
    }
}

void USkeletalMesh::CalculateInvRefMatrices() {
    const int32 NumRealBones = RefSkeleton.GetRawBoneNum();
    RefBasesInvMatrix.Empty(NumRealBones);
    TArray<FMatrix> ComposedRefPoseMatrices;
    ComposedRefPoseMatrices.SetNum(NumRealBones);
    const TArray<FTransform>& RefPose = RefSkeleton.GetRawRefBonePose();
    for (int32 BoneIndex = 0; BoneIndex < NumRealBones; BoneIndex++) {
        ComposedRefPoseMatrices[BoneIndex] = RefPose[BoneIndex].ToMatrixWithScale();
        if (BoneIndex > 0) {
            const int32 ParentIndex = RefSkeleton.GetRawParentIndex(BoneIndex);
            ComposedRefPoseMatrices[BoneIndex] = ComposedRefPoseMatrices[BoneIndex] * ComposedRefPoseMatrices[ParentIndex];
        }
        RefBasesInvMatrix.Add(FMatrix44f(ComposedRefPoseMatrices[BoneIndex].Inverse()));
    }
}

void USkeletalMesh::CalculateExtendedBounds() {
    FBoxSphereBounds CalculatedBounds = ImportedBounds;
    if (!NegativeBoundsExtension.IsZero() || !PositiveBoundsExtension.IsZero()) {
        const FVector Min = CalculatedBounds.Origin - CalculatedBounds.BoxExtent - NegativeBoundsExtension;
        const FVector Max = CalculatedBounds.Origin + CalculatedBounds.BoxExtent + PositiveBoundsExtension;
        CalculatedBounds.Origin = (Min + Max) / 2.0;
        CalculatedBounds.BoxExtent = (Max - Min) / 2.0;
        CalculatedBounds.SphereRadius = CalculatedBounds.BoxExtent.Size();
    }
    ExtendedBounds = CalculatedBounds;
}

USkeletalMeshSocket* USkeletalMesh::FindSocket(FName InSocketName) const {
    int32 DummyIndex;
    return FindSocketAndIndex(InSocketName, DummyIndex);
}

USkeletalMeshSocket* USkeletalMesh::FindSocketAndIndex(FName InSocketName, int32& OutIndex) const {
    OutIndex = INDEX_NONE;
    if (InSocketName.isNone()) return nullptr;
    for (int32 i = 0; i < Sockets.Num(); i++) {
        if (Sockets[i] != nullptr && Sockets[i]->SocketName == InSocketName) {
            OutIndex = i;
            return Sockets[i];
        }
    }
    // Then the skeleton's sockets, indexed after the mesh's.
    if (Skeleton != nullptr) {
        for (int32 i = 0; i < Skeleton->Sockets.Num(); i++) {
            if (Skeleton->Sockets[i] != nullptr && Skeleton->Sockets[i]->SocketName == InSocketName) {
                OutIndex = Sockets.Num() + i;
                return Skeleton->Sockets[i];
            }
        }
    }
    return nullptr;
}

int32 USkeletalMesh::NumSockets() const { return Sockets.Num() + (Skeleton ? Skeleton->Sockets.Num() : 0); }

USkeletalMeshSocket* USkeletalMesh::GetSocketByIndex(int32 Index) const {
    if (Sockets.IsValidIndex(Index)) return Sockets[Index];
    const int32 SkeletonIndex = Index - Sockets.Num();
    if (Skeleton != nullptr && Skeleton->Sockets.IsValidIndex(SkeletonIndex)) return Skeleton->Sockets[SkeletonIndex];
    return nullptr;
}

TArray<int32> USkeletalMesh::GetLODMaterialMap(int32 LODIndex) const {
    TArray<int32> Result;
    const FArrayProperty* LODInfo = FindFProperty<FArrayProperty>(GetClass(), FName("LODInfo"));
    if (LODInfo == nullptr) return Result;
    FScriptArrayHelper Helper(LODInfo, LODInfo->ContainerPtrToValuePtr<void>(this));
    if (!Helper.IsValidIndex(LODIndex)) return Result;
    const FStructProperty* InfoStruct = CastField<FStructProperty>(LODInfo->Inner);
    if (InfoStruct == nullptr) return Result;
    GetPropertyValue_InContainer(InfoStruct->Struct, Helper.GetRawPtr(LODIndex), FName("LODMaterialMap"), Result);
    return Result;
}

}
