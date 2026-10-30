#pragma once

#include <memory>

#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "ue5parse/runtime/Engine/Engine/SkinnedAsset.h"
#include "ue5parse/runtime/Engine/Engine/SkinnedAssetCommon.h"
#include "ue5parse/runtime/Engine/Rendering/SkeletalMeshRenderData.h"

#include "SkeletalMesh.generated.h"

namespace ue5parse::runtime {

class UMorphTarget;
class UAnimInstance;
class UBodySetup;

// USkeletalMesh (UE's Engine/SkeletalMesh.h): materials, the reference skeleton, sockets, and the cooked render data.
UCLASS()
class USkeletalMesh : public USkinnedAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    USkeleton* Skeleton = nullptr;
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
    UPhysicsAsset* PhysicsAsset = nullptr;
    UPROPERTY()
    UPhysicsAsset* ShadowPhysicsAsset = nullptr;
    UPROPERTY()
    TArray<UMorphTarget*> MorphTargets;
    UPROPERTY()
    TSubclassOf<UAnimInstance> PostProcessAnimBlueprint;
    UPROPERTY()
    int32 PostProcessAnimBPLODThreshold = INDEX_NONE;
    UPROPERTY()
    TArray<USkeletalMeshSocket*> Sockets;

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
    // A socket of the mesh, else of its skeleton (UE's FindSocket).
    UFUNCTION(BlueprintCallable)
    USkeletalMeshSocket* FindSocket(FName InSocketName) const override;
    USkeletalMeshSocket* FindSocketAndIndex(FName InSocketName, int32& OutIndex) const;
    const TArray<USkeletalMeshSocket*>& GetMeshOnlySocketList() const { return Sockets; }
    UFUNCTION(BlueprintCallable)
    int32 NumSockets() const;
    UFUNCTION(BlueprintCallable)
    USkeletalMeshSocket* GetSocketByIndex(int32 Index) const;
    int32 GetLODNum() const { return RenderData ? RenderData->LODRenderData.Num() : 0; }
    // LODInfo[LODIndex].LODMaterialMap: the material slot of each section when the LOD remaps them; empty otherwise.
    TArray<int32> GetLODMaterialMap(int32 LODIndex) const;
    // The inverse of each bone's component space reference pose (UE's GetRefBasesInvMatrix).
    const TArray<FMatrix44f>& GetRefBasesInvMatrix() const { return RefBasesInvMatrix; }

private:
    void CalculateInvRefMatrices();
    void CalculateExtendedBounds();

    FReferenceSkeleton RefSkeleton;
    std::unique_ptr<FSkeletalMeshRenderData> RenderData;
    TArray<FMatrix44f> RefBasesInvMatrix;
};

}
