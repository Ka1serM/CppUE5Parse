#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "SkinnedMeshComponent.generated.h"

namespace ue5parse::runtime {

class USkinnedAsset;
class USkeletalMesh;
class USkeletalMeshSocket;

enum class EVisibilityBasedAnimTickOption : uint8 {
    AlwaysTickPoseAndRefreshBones,
    AlwaysTickPose,
    OnlyTickMontagesWhenNotRendered,
    OnlyTickPoseWhenRendered,
};

UCLASS(Abstract)
class USkinnedMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkinnedAsset> SkinnedAsset = nullptr;
    UPROPERTY()
    TWeakObjectPtr<USkinnedMeshComponent> LeaderPoseComponent;
    UPROPERTY()
    EVisibilityBasedAnimTickOption VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
    bool bRecentlyRendered = true;

    USkinnedAsset* GetSkinnedAsset() const { return SkinnedAsset; }
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    bool ShouldTickPose() const { return VisibilityBasedAnimTickOption < EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered || bRecentlyRendered; }
    bool ShouldUpdateTransform() const {
        return bRecentlyRendered || VisibilityBasedAnimTickOption == EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
    UFUNCTION(BlueprintCallable)
    USkeletalMesh* GetSkeletalMesh_DEPRECATED() const;
    UFUNCTION(BlueprintCallable)
    void SetForcedLOD(int32 InNewForcedLOD) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetSkinnedAssetAndUpdate(USkinnedAsset* NewMesh, bool bReinitPose = true);
    UFUNCTION(BlueprintPure)
    FTransform GetDeltaTransformFromRefPose(FName BoneName, FName BaseName = FName()) const;
    UFUNCTION(BlueprintPure)
    FName GetSocketBoneName(FName InSocketName) const;
    UFUNCTION(BlueprintCallable)
    int32 GetBoneIndex(FName BoneName) const;
    UFUNCTION(BlueprintCallable)
    FName GetBoneName(int32 BoneIndex) const;
    UFUNCTION(BlueprintCallable)
    int32 GetNumBones() const;
    FMatrix GetBoneMatrix(int32 BoneIndex) const { return GetBoneTransform(BoneIndex).ToMatrixWithScale(); }
    bool IsBoneHidden(int32 BoneIndex) const { return false; }
    FTransform GetBoneTransform(int32 BoneIndex) const { return GetBoneTransform(BoneIndex, GetComponentTransform()); }
    FTransform GetBoneTransform(int32 BoneIndex, const FTransform& LocalToWorld) const;
    FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = ERelativeTransformSpace::RTS_World) const override;
    bool DoesSocketExist(FName InSocketName) const override;
    bool HasAnySockets() const override;
    void QuerySupportedSockets(TArray<FName>& OutSocketNames) const override;
    int32 GetNumMaterials() const override;
    UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
    int32 GetMaterialIndex(FName MaterialSlotName) const override;
};

}
