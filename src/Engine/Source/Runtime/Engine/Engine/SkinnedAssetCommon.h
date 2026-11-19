#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/SkeletalMeshVertexAttribute.h"
#include "Engine/Source/Runtime/Engine/MeshUVChannelInfo.h"
#include "Engine/Source/Runtime/Engine/SkeletalMeshReductionSettings.h"

#include "SkinnedAssetCommon.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

USTRUCT()
struct FSkeletalMaterial {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MaterialInterface = nullptr;
    UPROPERTY()
    FName MaterialSlotName;
    UPROPERTY()
    FMeshUVChannelInfo UVChannelData;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OverlayMaterialInterface = nullptr;
    FName ImportedMaterialSlotName;

    friend FArchive& operator<<(FArchive& Ar, FSkeletalMaterial& Material);
};

UENUM()
enum class ESkinCacheUsage : uint8 { Auto = 0, Disabled = uint8(-1), Enabled = 1 };

USTRUCT()
struct FSectionReference {
    GENERATED_BODY()
    UPROPERTY()
    int32 SectionIndex = INDEX_NONE;
};

USTRUCT()
struct FTriangleSortSettings {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<ETriangleSortOption> TriangleSorting = TRISORT_None;
    UPROPERTY()
    TEnumAsByte<ETriangleSortAxis> CustomLeftRightAxis = TSA_X_Axis;
    UPROPERTY()
    FName CustomLeftRightBoneName;
};

USTRUCT()
struct FSkeletalMeshLODInfo {
    GENERATED_BODY()
    UPROPERTY()
    FPerPlatformFloat ScreenSize{1.0f};
    UPROPERTY()
    float LODHysteresis = 0.0f;
    UPROPERTY()
    TArray<int32> LODMaterialMap;
    UPROPERTY()
    TArray<bool> bEnableShadowCasting_DEPRECATED;
    UPROPERTY()
    FSkeletalMeshBuildSettings BuildSettings;
    UPROPERTY()
    FSkeletalMeshOptimizationSettings ReductionSettings;
    UPROPERTY()
    TArray<FBoneReference> BonesToRemove;
    UPROPERTY()
    TArray<FBoneReference> BonesToPrioritize;
    UPROPERTY()
    TArray<FSectionReference> SectionsToPrioritize;
    UPROPERTY()
    float WeightOfPrioritization = 1.0f;
    UPROPERTY()
    TObjectPtr<UAnimSequence> BakePose = nullptr;
    UPROPERTY()
    TObjectPtr<UAnimSequence> BakePoseOverride = nullptr;
    UPROPERTY()
    FString SourceImportFilename;
    UPROPERTY()
    ESkinCacheUsage SkinCacheUsage = ESkinCacheUsage::Auto;
    UPROPERTY()
    float MorphTargetPositionErrorTolerance = 20.0f;
    UPROPERTY()
    TArray<FSkeletalMeshVertexAttributeInfo> VertexAttributes;
    UPROPERTY()
    bool bHasBeenSimplified = false;
    UPROPERTY()
    bool bHasPerLODVertexColors = false;
    UPROPERTY()
    bool bAllowCPUAccess = false;
    UPROPERTY()
    bool bBuildHalfEdgeBuffers = false;
    UPROPERTY()
    bool bAllowMeshDeformer = true;
    UPROPERTY()
    bool bSupportUniformlyDistributedSampling = false;
    UPROPERTY()
    float DisplayFactor = 0.0f;
    UPROPERTY()
    TArray<TEnumAsByte<ETriangleSortOption>> TriangleSorting_DEPRECATED;
    UPROPERTY()
    TArray<FTriangleSortSettings> TriangleSortSettings;
    UPROPERTY()
    TArray<FName> RemovedBones;
};

}
