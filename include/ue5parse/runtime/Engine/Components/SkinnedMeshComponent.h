#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "ue5parse/runtime/Engine/Components/MeshComponent.h"

#include "SkinnedMeshComponent.generated.h"

namespace ue5parse::runtime {

class USkinnedAsset;
class USkeletalMesh;
class USkeletalMeshSocket;

// When a mesh that does not render ticks its pose and refreshes its bones (UE's EVisibilityBasedAnimTickOption).
enum class EVisibilityBasedAnimTickOption : uint8 {
    AlwaysTickPoseAndRefreshBones,
    AlwaysTickPose,
    OnlyTickMontagesWhenNotRendered,
    OnlyTickPoseWhenRendered,
};

// USkinnedMeshComponent: a placed skinned mesh (UE's Components/SkinnedMeshComponent.h).
UCLASS(Abstract)
class USkinnedMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    USkinnedAsset* SkinnedAsset = nullptr;
    // The component whose pose this one follows, when set (UE's leader pose component).
    UPROPERTY()
    TWeakObjectPtr<USkinnedMeshComponent> LeaderPoseComponent;
    UPROPERTY()
    EVisibilityBasedAnimTickOption VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
    // Whether the mesh rendered at the start of its tick. Nothing draws, so a mesh renders when it shows in the game
    // (ShouldRender) instead of UE's last render time.
    bool bRecentlyRendered = true;

    USkinnedAsset* GetSkinnedAsset() const { return SkinnedAsset; }
    // Sets bRecentlyRendered first: the pose tick and the bone refresh of the subclass depend on it.
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    // UE's ShouldTickPose and ShouldUpdateTransform: the visibility based parts.
    bool ShouldTickPose() const {
        return VisibilityBasedAnimTickOption < EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered || bRecentlyRendered;
    }
    bool ShouldUpdateTransform() const {
        return bRecentlyRendered || VisibilityBasedAnimTickOption == EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
    UFUNCTION(BlueprintCallable)
    USkeletalMesh* GetSkeletalMesh_DEPRECATED() const;
    // Changes the asset; nothing happens when it is the asset already.
    UFUNCTION(BlueprintCallable)
    virtual void SetSkinnedAssetAndUpdate(USkinnedAsset* NewMesh, bool bReinitPose = true);
    UFUNCTION(BlueprintPure)
    FTransform GetDeltaTransformFromRefPose(FName BoneName, FName BaseName = FName()) const;
    UFUNCTION(BlueprintPure)
    FName GetSocketBoneName(FName InSocketName) const;
    // The bone's world transform from the component space pose; the component's transform when the pose has no such bone.
    FTransform GetBoneTransform(int32 BoneIndex) const;
    FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = ERelativeTransformSpace::RTS_World) const override;
    bool DoesSocketExist(FName InSocketName) const override;
    bool HasAnySockets() const override;
    void QuerySupportedSockets(TArray<FName>& OutSocketNames) const override;
    int32 GetNumMaterials() const override;
    UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
    int32 GetMaterialIndex(FName MaterialSlotName) const override;
};

}
