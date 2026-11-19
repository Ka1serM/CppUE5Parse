#pragma once

#include <memory>

#include "Engine/Source/Runtime/Core/Math/Axis.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAsset.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSampling.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSourceModel.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAssetCommon.h"
#include "Engine/Source/Runtime/Engine/Rendering/SkinWeightProfile.h"
#include "Engine/Source/Runtime/Engine/Rendering/SkeletalMeshRenderData.h"

#include "SkeletalMesh.generated.h"

namespace ue5parse::runtime {

class UMorphTarget;
class UAnimInstance;
class UBodySetup;
class UNodeMappingContainer;
class UClothingAssetBase;
class UMeshDeformer;
class UMeshDeformerCollection;

UENUM()
enum class EClothLODBiasMode : uint8 { MappingsToSameLOD, MappingsToMinLOD, MappingsToAnyLOD };

UENUM()
enum class ESkeletalMeshBuildVersion : int32 { Invalid = -1, Initial = 0, PolygonWireframe = 1 };

USTRUCT()
struct FBoneMirrorInfo {
    GENERATED_BODY()
    UPROPERTY()
    int32 SourceIndex = 0;
    UPROPERTY()
    TEnumAsByte<EAxis::Type> BoneFlipAxis = EAxis::None;
};

USTRUCT()
struct FClothPhysicsProperties_Legacy {
    GENERATED_BODY()
    UPROPERTY()
    float VerticalResistance = 0.0f;
    UPROPERTY()
    float HorizontalResistance = 0.0f;
    UPROPERTY()
    float BendResistance = 0.0f;
    UPROPERTY()
    float ShearResistance = 0.0f;
    UPROPERTY()
    float Friction = 0.0f;
    UPROPERTY()
    float Damping = 0.0f;
    UPROPERTY()
    float TetherStiffness = 0.0f;
    UPROPERTY()
    float TetherLimit = 0.0f;
    UPROPERTY()
    float Drag = 0.0f;
    UPROPERTY()
    float StiffnessFrequency = 0.0f;
    UPROPERTY()
    float GravityScale = 0.0f;
    UPROPERTY()
    float MassScale = 0.0f;
    UPROPERTY()
    float InertiaBlend = 0.0f;
    UPROPERTY()
    float SelfCollisionThickness = 0.0f;
    UPROPERTY()
    float SelfCollisionSquashScale = 0.0f;
    UPROPERTY()
    float SelfCollisionStiffness = 0.0f;
    UPROPERTY()
    float SolverFrequency = 0.0f;
    UPROPERTY()
    float FiberCompression = 0.0f;
    UPROPERTY()
    float FiberExpansion = 0.0f;
    UPROPERTY()
    float FiberResistance = 0.0f;
};

USTRUCT()
struct FClothingAssetData_Legacy {
    GENERATED_BODY()
    UPROPERTY()
    FName AssetName;
    UPROPERTY()
    FString ApexFileName;
    UPROPERTY()
    bool bClothPropertiesChanged = false;
    UPROPERTY()
    FClothPhysicsProperties_Legacy PhysicsProperties;
    UPROPERTY()
    TArray<int32> ApexToUnrealBoneMapping;
};

USTRUCT()
struct FMorphTargetMap {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TObjectPtr<UMorphTarget> MorphTarget = nullptr;
};

UCLASS()
class USkeletalMesh : public USkinnedAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeleton> Skeleton = nullptr;
    UPROPERTY()
    FBoxSphereBounds Bounds;
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
    TArray<FBoneMirrorInfo> SkelMirrorTable;
    UPROPERTY()
    TEnumAsByte<EAxis::Type> SkelMirrorAxis = EAxis::None;
    UPROPERTY()
    TEnumAsByte<EAxis::Type> SkelMirrorFlipAxis = EAxis::None;
    UPROPERTY()
    TArray<FSkeletalMeshLODInfo> LODInfo;
    UPROPERTY()
    TArray<FSkeletalMeshSourceModel> SourceModels;
    UPROPERTY()
    bool bUseFullPrecisionUVs = false;
    UPROPERTY()
    bool bUseHighPrecisionTangentBasis = false;
    UPROPERTY()
    bool bHasBeenSimplified = false;
    UPROPERTY()
    bool bHasVertexColors = false;
    UPROPERTY()
    bool bEnablePerPolyCollision = false;
    UPROPERTY()
    bool bSupportRayTracing = true;
    UPROPERTY()
    bool bAlwaysAnimateHiddenBones = false;
    UPROPERTY()
    TObjectPtr<UPhysicsAsset> PhysicsAsset = nullptr;
    UPROPERTY()
    TObjectPtr<UPhysicsAsset> ShadowPhysicsAsset = nullptr;
    UPROPERTY()
    float StreamingDistanceMultiplier = 1.0f;
    UPROPERTY()
    TArray<FMorphTargetMap> MorphTargetTable_DEPRECATED;
    UPROPERTY()
    TArray<TObjectPtr<UMorphTarget>> MorphTargets;
    UPROPERTY()
    TArray<FClothingAssetData_Legacy> ClothingAssets;
    UPROPERTY()
    TArray<TObjectPtr<UClothingAssetBase>> MeshClothingAssets;
    UPROPERTY()
    TArray<TObjectPtr<UNodeMappingContainer>> NodeMappingData;
    UPROPERTY()
    TSubclassOf<UAnimInstance> PostProcessAnimBlueprint;
    UPROPERTY()
    int32 PostProcessAnimBPLODThreshold = INDEX_NONE;
    UPROPERTY()
    TArray<TObjectPtr<USkeletalMeshSocket>> Sockets;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;
    UPROPERTY()
    TObjectPtr<UBodySetup> BodySetup = nullptr;
    UPROPERTY()
    FSkeletalMeshSamplingInfo SamplingInfo;
    UPROPERTY()
    FPerPlatformInt MinLod;
    UPROPERTY()
    FPerQualityLevelInt MinQualityLevelLOD;
    UPROPERTY()
    FPerPlatformBool DisableBelowMinLodStripping;
    UPROPERTY()
    TArray<FSkinWeightProfileInfo> SkinWeightProfiles;
    UPROPERTY()
    int32 RayTracingMinLOD = 0;
    UPROPERTY()
    EClothLODBiasMode ClothLODBiasMode = EClothLODBiasMode::MappingsToSameLOD;
    UPROPERTY()
    TObjectPtr<UMeshDeformer> DefaultMeshDeformer = nullptr;
    UPROPERTY()
    TObjectPtr<UMeshDeformerCollection> TargetMeshDeformers = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OverlayMaterial = nullptr;
    UPROPERTY()
    float OverlayMaterialMaxDrawDistance = 0.0f;
    UPROPERTY()
    FMeshNaniteSettings NaniteSettings;
    UPROPERTY()
    TEnumAsByte<EAxis::Type> ForwardAxis = EAxis::Y;
    UPROPERTY()
    ESkeletalMeshBuildVersion BuildVersion = ESkeletalMeshBuildVersion::Initial;

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
    const FSkeletalMeshLODInfo* GetLODInfo(int32 Index) const;

private:
    void CalculateInvRefMatrices();
    void CalculateExtendedBounds();

    FReferenceSkeleton RefSkeleton;
    std::unique_ptr<FSkeletalMeshRenderData> RenderData;
    TArray<FMatrix44f> RefBasesInvMatrix;
};

}
