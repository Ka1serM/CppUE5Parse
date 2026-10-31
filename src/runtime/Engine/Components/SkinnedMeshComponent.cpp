#include "ue5parse/runtime/Engine/Components/SkinnedMeshComponent.h"

#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "ue5parse/runtime/Engine/Engine/SkinnedAsset.h"
#include "ue5parse/runtime/Engine/Engine/SkinnedAssetCommon.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's Components/SkinnedMeshComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

int32 USkinnedMeshComponent::GetNumMaterials() const { return SkinnedAsset ? SkinnedAsset->GetMaterials().Num() : 0; }

int32 USkinnedMeshComponent::GetMaterialIndex(FName MaterialSlotName) const {
    if (SkinnedAsset == nullptr) return INDEX_NONE;
    const TArray<FSkeletalMaterial>& SkeletalMeshMaterials = SkinnedAsset->GetMaterials();
    for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMeshMaterials.Num(); ++MaterialIndex)
        if (SkeletalMeshMaterials[MaterialIndex].MaterialSlotName == MaterialSlotName) return MaterialIndex;
    return INDEX_NONE;
}

UMaterialInterface* USkinnedMeshComponent::GetMaterial(int32 MaterialIndex) const {
    if (UMaterialInterface* Override = Super::GetMaterial(MaterialIndex)) return Override;
    if (SkinnedAsset == nullptr || !SkinnedAsset->GetMaterials().IsValidIndex(MaterialIndex)) return nullptr;
    return SkinnedAsset->GetMaterials()[MaterialIndex].MaterialInterface;
}

USkeletalMesh* USkinnedMeshComponent::GetSkeletalMesh_DEPRECATED() const { return Cast<USkeletalMesh>(GetSkinnedAsset()); }

void USkinnedMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    bRecentlyRendered = ShouldRender();
}

void USkinnedMeshComponent::SetSkinnedAssetAndUpdate(USkinnedAsset* NewMesh, bool /*bReinitPose*/) {
    if (NewMesh == GetSkinnedAsset()) return;
    SkinnedAsset = NewMesh;
}

FName USkinnedMeshComponent::GetSocketBoneName(FName InSocketName) const {
    if (SkinnedAsset == nullptr) return NAME_None;
    if (USkeletalMeshSocket* Socket = SkinnedAsset->FindSocket(InSocketName)) return Socket->BoneName;
    const FReferenceSkeleton& Skeleton = SkinnedAsset->GetRefSkeleton();
    return Skeleton.FindBoneIndex(InSocketName) != INDEX_NONE ? InSocketName : NAME_None;
}

FTransform USkinnedMeshComponent::GetBoneTransform(int32 BoneIndex, const FTransform& LocalToWorld) const {
    const USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(this);
    if (Skeletal == nullptr || !Skeletal->GetComponentSpaceTransforms().IsValidIndex(BoneIndex)) return LocalToWorld;
    return Skeletal->GetComponentSpaceTransforms()[BoneIndex] * LocalToWorld;
}

FTransform USkinnedMeshComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const {
    FTransform OutSocketTransform = GetComponentTransform();
    if (!InSocketName.isNone() && SkinnedAsset != nullptr) {
        const FReferenceSkeleton& Skeleton = SkinnedAsset->GetRefSkeleton();
        if (const USkeletalMeshSocket* Socket = SkinnedAsset->FindSocket(InSocketName)) {
            const FTransform SocketLocalTransform = Socket->GetSocketLocalTransform();
            if (TransformSpace == ERelativeTransformSpace::RTS_ParentBoneSpace) return SocketLocalTransform;
            const int32 SocketBoneIndex = Skeleton.FindBoneIndex(Socket->BoneName);
            if (SocketBoneIndex != INDEX_NONE) OutSocketTransform = SocketLocalTransform * GetBoneTransform(SocketBoneIndex);
        } else if (const int32 BoneIndex = Skeleton.FindBoneIndex(InSocketName); BoneIndex != INDEX_NONE) {
            OutSocketTransform = GetBoneTransform(BoneIndex);
            if (TransformSpace == ERelativeTransformSpace::RTS_ParentBoneSpace) {
                const int32 ParentIndex = Skeleton.GetParentIndex(BoneIndex);
                return OutSocketTransform.GetRelativeTransform(ParentIndex != INDEX_NONE ? GetBoneTransform(ParentIndex) : GetComponentTransform());
            }
        }
    }
    switch (TransformSpace) {
    case ERelativeTransformSpace::RTS_Actor:
        if (const AActor* Actor = GetOwner()) return OutSocketTransform.GetRelativeTransform(Actor->GetActorTransform());
        break;
    case ERelativeTransformSpace::RTS_Component: return OutSocketTransform.GetRelativeTransform(GetComponentTransform());
    default: break;
    }
    return OutSocketTransform;
}

bool USkinnedMeshComponent::DoesSocketExist(FName InSocketName) const {
    if (SkinnedAsset == nullptr) return false;
    return SkinnedAsset->FindSocket(InSocketName) != nullptr || SkinnedAsset->GetRefSkeleton().FindBoneIndex(InSocketName) != INDEX_NONE;
}

bool USkinnedMeshComponent::HasAnySockets() const {
    if (SkinnedAsset == nullptr) return false;
    const USkeletalMesh* Mesh = Cast<USkeletalMesh>(SkinnedAsset);
    return SkinnedAsset->GetRefSkeleton().GetNum() > 0 || (Mesh != nullptr && Mesh->NumSockets() > 0);
}

void USkinnedMeshComponent::QuerySupportedSockets(TArray<FName>& OutSocketNames) const {
    if (SkinnedAsset == nullptr) return;
    if (const USkeletalMesh* Mesh = Cast<USkeletalMesh>(SkinnedAsset))
        for (int32 Index = 0; Index < Mesh->NumSockets(); ++Index)
            if (const USkeletalMeshSocket* Socket = Mesh->GetSocketByIndex(Index)) OutSocketNames.Add(Socket->SocketName);
    const FReferenceSkeleton& Skeleton = SkinnedAsset->GetRefSkeleton();
    for (int32 BoneIndex = 0; BoneIndex < Skeleton.GetNum(); ++BoneIndex) OutSocketNames.Add(Skeleton.GetBoneName(BoneIndex));
}

FTransform USkinnedMeshComponent::GetDeltaTransformFromRefPose(FName BoneName, FName BaseName) const {
    if (SkinnedAsset == nullptr) return FTransform::Identity;
    const FReferenceSkeleton& Skeleton = SkinnedAsset->GetRefSkeleton();
    const int32 BoneIndex = Skeleton.FindBoneIndex(BoneName);
    if (BoneIndex == INDEX_NONE) return FTransform::Identity;
    int32 BaseIndex = BaseName == NAME_None ? Skeleton.GetParentIndex(BoneIndex) : Skeleton.FindBoneIndex(BaseName);

    auto RefComponentSpace = [&Skeleton](int32 Index) {
        FTransform Result = Skeleton.GetRefBonePose()[Index];
        for (int32 Parent = Skeleton.GetParentIndex(Index); Parent != INDEX_NONE; Parent = Skeleton.GetParentIndex(Parent))
            Result = Result * Skeleton.GetRefBonePose()[Parent];
        return Result;
    };

    const FTransform ReferenceBone = RefComponentSpace(BoneIndex);
    const FTransform ReferenceBase = BaseIndex != INDEX_NONE ? RefComponentSpace(BaseIndex) : FTransform::Identity;
    FTransform CurrentBone = ReferenceBone;
    FTransform CurrentBase = ReferenceBase;
    if (const USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(this)) {
        const TArray<FTransform>& Transforms = Skeletal->GetComponentSpaceTransforms();
        if (Transforms.IsValidIndex(BoneIndex)) CurrentBone = Transforms[BoneIndex];
        if (BaseIndex != INDEX_NONE && Transforms.IsValidIndex(BaseIndex)) CurrentBase = Transforms[BaseIndex];
    }

    const FTransform ReferenceRelative =
        BaseIndex != INDEX_NONE ? ReferenceBone.GetRelativeTransform(ReferenceBase) : ReferenceBone;
    const FTransform CurrentRelative =
        BaseIndex != INDEX_NONE ? CurrentBone.GetRelativeTransform(CurrentBase) : CurrentBone;
    return CurrentRelative.GetRelativeTransform(ReferenceRelative);
}

}
