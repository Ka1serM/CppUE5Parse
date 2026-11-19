#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAssetCommon.h"

#include "SkinnedMeshComponent.generated.h"

namespace ue5parse::runtime {

class USkinnedAsset;
class USkeletalMesh;
class USkeletalMeshSocket;
class UMeshDeformer;
class UMeshDeformerInstanceSettings;
class UPhysicsAsset;

UENUM()
enum class EVisibilityBasedAnimTickOption : uint8 {
    AlwaysTickPoseAndRefreshBones,
    AlwaysTickPose,
    OnlyTickMontagesWhenNotRendered,
    OnlyTickPoseWhenRendered,
};

USTRUCT()
struct FVertexOffsetUsage {
    GENERATED_BODY()
    UPROPERTY()
    int32 Usage = 0;
};

UCLASS(Abstract)
class USkinnedMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;
    UPROPERTY()
    TObjectPtr<USkinnedAsset> SkinnedAsset = nullptr;
    UPROPERTY()
    TWeakObjectPtr<USkinnedMeshComponent> LeaderPoseComponent;
    UPROPERTY()
    TArray<ESkinCacheUsage> SkinCacheUsage;
    UPROPERTY()
    TArray<FVertexOffsetUsage> VertexOffsetUsage;
    UPROPERTY()
    bool bSetMeshDeformer = false;
    UPROPERTY()
    TObjectPtr<UMeshDeformer> MeshDeformer = nullptr;
    UPROPERTY()
    bool bAlwaysUseMeshDeformer = false;
    UPROPERTY()
    TObjectPtr<UMeshDeformerInstanceSettings> MeshDeformerInstanceSettings = nullptr;
    UPROPERTY()
    TObjectPtr<UPhysicsAsset> PhysicsAssetOverride = nullptr;
    UPROPERTY()
    int32 ForcedLodModel = 0;
    UPROPERTY()
    int32 MinLodModel = 0;
    UPROPERTY()
    int32 PredictedLODLevel = 0;
    UPROPERTY()
    int32 OldPredictedLODLevel = 0;
    UPROPERTY()
    float MaxDistanceFactor = 0.0f;
    UPROPERTY()
    float StreamingDistanceMultiplier = 1.0f;
    UPROPERTY()
    float NanitePixelProgrammableDistance = 0.0f;
    UPROPERTY()
    int32 WorldPositionOffsetDisableDistance = 0;
    UPROPERTY()
    uint8 bEvaluateWorldPositionOffset : 1 = true;
    UPROPERTY()
    uint8 bDisallowNanite : 1 = false;
    UPROPERTY()
    uint8 bForceDisableNanite : 1 = false;
    UPROPERTY()
    EVisibilityBasedAnimTickOption VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    UPROPERTY()
    uint8 bSkinWeightProfileOnRenderThread : 1 = false;
    UPROPERTY()
    uint8 bOverrideMinLod : 1 = false;
    UPROPERTY()
    uint8 bUseBoundsFromLeaderPoseComponent : 1 = false;
    UPROPERTY()
    uint8 bIncludeComponentLocationIntoBounds : 1 = false;
    UPROPERTY()
    uint8 bForceWireframe : 1 = false;
    UPROPERTY()
    uint8 bDisableMorphTarget : 1 = false;
    UPROPERTY()
    uint8 bHideSkin : 1 = false;
    UPROPERTY()
    uint8 bPerBoneMotionBlur : 1 = true;
    UPROPERTY()
    uint8 bComponentUseFixedSkelBounds : 1 = false;
    UPROPERTY()
    uint8 bConsiderAllBodiesForBounds : 1 = false;
    UPROPERTY()
    uint8 bSyncAttachParentLOD : 1 = true;
    UPROPERTY()
    uint8 bCastCapsuleDirectShadow : 1 = false;
    UPROPERTY()
    uint8 bCastCapsuleIndirectShadow : 1 = false;
    UPROPERTY()
    uint8 bUseScreenRenderStateForUpdate : 1 = false;
    UPROPERTY()
    uint8 bEnableUpdateRateOptimizations : 1 = false;
    UPROPERTY()
    uint8 bDisplayDebugUpdateRateOptimizations : 1 = false;
    UPROPERTY()
    uint8 bRenderStatic : 1 = false;
    UPROPERTY()
    uint8 bRenderShadowStatic : 1 = false;
    UPROPERTY()
    uint8 bIgnoreLeaderPoseComponentLOD : 1 = false;
    UPROPERTY()
    uint8 bSortTriangles : 1 = false;
    UPROPERTY()
    uint8 bChartDistanceFactor : 1 = false;
    UPROPERTY()
    float CapsuleIndirectShadowMinVisibility = 0.1f;
    bool bRecentlyRendered = true;

    explicit USkinnedMeshComponent(const FObjectInitializer& ObjectInitializer);

    USkinnedAsset* GetSkinnedAsset() const { return SkinnedAsset; }
    void PostLoad() override;
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
