#pragma once

#include <memory>

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAsset.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAssetCommon.h"
#include "Engine/Source/Runtime/Engine/Rendering/SkeletalMeshRenderData.h"

#include "SkeletalMesh.generated.h"

namespace ue5parse::runtime {

class UMorphTarget;
class UAnimInstance;
class UBodySetup;

UCLASS()
class USkeletalMesh : public USkinnedAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeleton> Skeleton = nullptr;
    UPROPERTY()
    FBoxSphereBounds ImportedBounds;
    UPROPERTY()
    FBoxSphereBounds ExtendedBounds;
    UPROPERTY()
    FVector PositiveBoundsExtension = FVector::ZeroVector;
    UPROPERTY()
    FVector NegativeBoundsExtension = FVector::ZeroVector;
    UPROPERTY()
    TArray<FSkeletalMaterial> Materials;
    UPROPERTY()
    bool bHasVertexColors = false;
    UPROPERTY()
    TObjectPtr<UPhysicsAsset> PhysicsAsset = nullptr;
    UPROPERTY()
    TObjectPtr<UPhysicsAsset> ShadowPhysicsAsset = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMorphTarget>> MorphTargets;
    UPROPERTY()
    TSubclassOf<UAnimInstance> PostProcessAnimBlueprint;
    UPROPERTY()
    int32 PostProcessAnimBPLODThreshold = INDEX_NONE;
    UPROPERTY()
    TArray<TObjectPtr<USkeletalMeshSocket>> Sockets;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;

    void Serialize(FArchive& Ar) override;
    void PostLoad() override;
    USkeleton* GetSkeleton() const override { return Skeleton; }
    const FReferenceSkeleton& GetRefSkeleton() const override { return RefSkeleton; }
    FReferenceSkeleton& GetRefSkeleton() { return RefSkeleton; }
    UPhysicsAsset* GetPhysicsAsset() const override { return PhysicsAsset; }
    FSkeletalMeshRenderData* GetResourceForRendering() const override { return RenderData.get(); }
    UFUNCTION(BlueprintGetter)
    const TArray<FSkeletalMaterial>& GetMaterials() const override { return Materials; }
    TSubclassOf<UAnimInstance> GetPostProcessAnimBlueprint() const { return PostProcessAnimBlueprint; }
    bool ShouldEvaluatePostProcessAnimBP(int32 LODLevel) const {
        return PostProcessAnimBPLODThreshold == INDEX_NONE || LODLevel <= PostProcessAnimBPLODThreshold;
    }
    FBoxSphereBounds GetImportedBounds() const { return ImportedBounds; }
    FBoxSphereBounds GetBounds() const override { return ExtendedBounds; }
    UFUNCTION(BlueprintCallable)
    USkeletalMeshSocket* FindSocket(FName InSocketName) const override;
    USkeletalMeshSocket* FindSocketAndIndex(FName InSocketName, int32& OutIndex) const;
    const TArray<TObjectPtr<USkeletalMeshSocket>>& GetMeshOnlySocketList() const { return Sockets; }
    UFUNCTION(BlueprintCallable)
    int32 NumSockets() const;
    UFUNCTION(BlueprintCallable)
    USkeletalMeshSocket* GetSocketByIndex(int32 Index) const;
    TArray<int32> GetLODMaterialMap(int32 LODIndex) const;

private:
    void CalculateInvRefMatrices();
    void CalculateExtendedBounds();

    FReferenceSkeleton RefSkeleton;
    std::unique_ptr<FSkeletalMeshRenderData> RenderData;
    TArray<FMatrix44f> RefBasesInvMatrix;
};

}
