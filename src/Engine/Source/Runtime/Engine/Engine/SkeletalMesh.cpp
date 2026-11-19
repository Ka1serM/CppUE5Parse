#include "Engine/Source/Runtime/Engine/Engine/SkeletalMesh.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Animation/Skeleton.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodySetup.h"

namespace ue5parse::runtime {

namespace {

const FReferenceSkeleton& GetEmptyRefSkeleton() {
    static const FReferenceSkeleton Empty;
    return Empty;
}

}

const FReferenceSkeleton& USkinnedAsset::GetRefSkeleton() const {
    return GetEmptyRefSkeleton();
}

const TArray<FSkeletalMaterial>& USkinnedAsset::GetMaterials() const {
    static const TArray<FSkeletalMaterial> Empty;
    return Empty;
}

FArchive& operator<<(FArchive& Ar, FSkeletalMaterial& Material) {
    UObject* Interface = nullptr;
    Ar << Interface;
    Material.MaterialInterface = Cast<UMaterialInterface>(Interface);
    if (Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::RefactorMeshEditorMaterials, EGame::GAME_UE4_14)) {
        Ar << Material.MaterialSlotName;
        //! Before UE 4.22 cooked packages omit ImportedMaterialSlotName.
        bool bSerializeImportedMaterialSlotName = false;
        if (Ar.CustomVerAtLeast(FCoreObjectVersion::GUID, FCoreObjectVersion::SkeletalMaterialEditorDataStripping, EGame::GAME_UE4_22))
            Ar << bSerializeImportedMaterialSlotName;
        if (bSerializeImportedMaterialSlotName)
            Ar << Material.ImportedMaterialSlotName;
    } else {
        bool bDeprecatedFlag = false;
        if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::MOVE_SKELETALMESH_SHADOWCASTING)
            Ar << bDeprecatedFlag;
        if (Ar.CustomVerAtLeast(FRecomputeTangentCustomVersion::GUID, FRecomputeTangentCustomVersion::RuntimeRecomputeTangent, EGame::GAME_UE4_12))
            Ar << bDeprecatedFlag;
    }
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::TextureStreamingMeshUVChannelData, EGame::GAME_UE4_14))
        Ar << Material.UVChannelData;
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::MeshMaterialSlotOverlayMaterialAdded, EGame::GAME_UE5_6))
        return Ar;
    UObject* OverlayInterface = nullptr;
    Ar << OverlayInterface;
    Material.OverlayMaterialInterface = Cast<UMaterialInterface>(OverlayInterface);
    return Ar;
}

void USkeletalMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    Ar << ImportedBounds << Materials << RefSkeleton;
    if (Ar.IsError())
        return;
    RefSkeleton.RebuildRefSkeleton(Skeleton, false);
    if (!StripFlags.IsEditorDataStripped()) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "USkeletalMesh.EditorData");
        return;
    }
    //! Before UE 4.19 the imported resource holds the render data, without a cooked flag.
    bool bCooked = true;
    if (Ar.CustomVerAtLeast(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::SplitModelAndRenderData, EGame::GAME_UE4_19)) {
        Ar << bCooked;
        bCooked = bCooked && !IsTemplate();
    }
    if (bCooked) {
        RenderData = std::make_unique<FSkeletalMeshRenderData>();
        RenderData->Serialize(Ar, bHasVertexColors);
        if (Ar.IsError())
            return;
    }
    TArray<UObject*> DummyObjs;
    Ar << DummyObjs;
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::TextureStreamingMeshUVChannelData, EGame::GAME_UE4_14)) {
        TArray<float> CachedStreamingTextureFactors;
        Ar << CachedStreamingTextureFactors;
    }
    //! UE 4.6 moves the per poly collision body setup from the component to the mesh.
    if (!bEnablePerPolyCollision || Ar.GetGame() < EGame::GAME_UE4_6)
        return;
    UObject* LocalBodySetup = nullptr;
    Ar << LocalBodySetup;
    BodySetup = Cast<UBodySetup>(LocalBodySetup);
}

void USkeletalMesh::PostLoad() {
    Super::PostLoad();
    if (!LODInfo.IsEmpty()) {
        SourceModels.SetNum(LODInfo.Num());
        for (int32 LODIndex = 0; LODIndex < LODInfo.Num(); LODIndex++)
            static_cast<FSkeletalMeshLODInfo&>(SourceModels[LODIndex]) = LODInfo[LODIndex];
        LODInfo.Empty();
    }
    RefSkeleton.RebuildRefSkeleton(Skeleton, false);
    CalculateInvRefMatrices();
    CalculateExtendedBounds();
    if (RenderData == nullptr)
        return;
    for (int32 LODIndex = 0; LODIndex < RenderData->LODRenderData.Num(); LODIndex++) {
        const FSkeletalMeshLODInfo* Info = GetLODInfo(LODIndex);
        if (Info == nullptr)
            continue;
        TArray<FSkelMeshRenderSection>& Sections = RenderData->LODRenderData[LODIndex].RenderSections;
        for (int32 SectionIndex = 0; SectionIndex < Sections.Num() && SectionIndex < Info->LODMaterialMap.Num(); SectionIndex++) {
            const int32 Mapped = Info->LODMaterialMap[SectionIndex];
            if (Materials.IsValidIndex(Mapped))
                Sections[SectionIndex].MaterialIndex = static_cast<uint16>(Mapped);
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
    if (InSocketName.isNone())
        return nullptr;
    for (int32 i = 0; i < Sockets.Num(); i++) {
        if (Sockets[i] != nullptr && Sockets[i]->SocketName == InSocketName) {
            OutIndex = i;
            return Sockets[i];
        }
    }
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

int32 USkeletalMesh::NumSockets() const {
    return Sockets.Num() + (Skeleton ? Skeleton->Sockets.Num() : 0);
}

USkeletalMeshSocket* USkeletalMesh::GetSocketByIndex(int32 Index) const {
    if (Sockets.IsValidIndex(Index))
        return Sockets[Index];
    const int32 SkeletonIndex = Index - Sockets.Num();
    if (Skeleton != nullptr && Skeleton->Sockets.IsValidIndex(SkeletonIndex))
        return Skeleton->Sockets[SkeletonIndex];
    return nullptr;
}

const FSkeletalMeshLODInfo* USkeletalMesh::GetLODInfo(int32 Index) const {
    return SourceModels.IsValidIndex(Index) ? &SourceModels[Index] : nullptr;
}

}
