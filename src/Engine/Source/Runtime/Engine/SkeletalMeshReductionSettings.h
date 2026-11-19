#pragma once

#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "SkeletalMeshReductionSettings.generated.h"

namespace ue5parse::runtime {

class UAnimSequence;

UENUM()
enum SkeletalMeshOptimizationType : int32 { SMOT_NumOfTriangles, SMOT_MaxDeviation, SMOT_TriangleOrDeviation, SMOT_MAX };

UENUM()
enum SkeletalMeshOptimizationImportance : int32 { SMOI_Off, SMOI_Lowest, SMOI_Low, SMOI_Normal, SMOI_High, SMOI_Highest, SMOI_MAX };

UENUM()
enum SkeletalMeshTerminationCriterion : int32 {
    SMTC_NumOfTriangles,
    SMTC_NumOfVerts,
    SMTC_TriangleOrVert,
    SMTC_AbsNumOfTriangles,
    SMTC_AbsNumOfVerts,
    SMTC_AbsTriangleOrVert,
    SMTC_MAX
};

USTRUCT()
struct FSkeletalMeshOptimizationSettings {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<SkeletalMeshTerminationCriterion> TerminationCriterion = SMTC_NumOfTriangles;
    UPROPERTY()
    float NumOfTrianglesPercentage = 0.5f;
    UPROPERTY()
    float NumOfVertPercentage = 0.5f;
    UPROPERTY()
    uint32 MaxNumOfTriangles = 4;
    UPROPERTY()
    uint32 MaxNumOfVerts = 6;
    UPROPERTY()
    float MaxDeviationPercentage = 0.5f;
    UPROPERTY()
    TEnumAsByte<SkeletalMeshOptimizationType> ReductionMethod = SMOT_NumOfTriangles;
    UPROPERTY()
    TEnumAsByte<SkeletalMeshOptimizationImportance> SilhouetteImportance = SMOI_Normal;
    UPROPERTY()
    TEnumAsByte<SkeletalMeshOptimizationImportance> TextureImportance = SMOI_Normal;
    UPROPERTY()
    TEnumAsByte<SkeletalMeshOptimizationImportance> ShadingImportance = SMOI_Normal;
    UPROPERTY()
    TEnumAsByte<SkeletalMeshOptimizationImportance> SkinningImportance = SMOI_Normal;
    UPROPERTY()
    bool bRemapMorphTargets = false;
    UPROPERTY()
    bool bRecalcNormals = true;
    UPROPERTY()
    float WeldingThreshold = 0.1f;
    UPROPERTY()
    float NormalsThreshold = 60.0f;
    UPROPERTY()
    int32 MaxBonesPerVertex = 4;
    UPROPERTY()
    bool bEnforceBoneBoundaries = false;
    UPROPERTY()
    bool bMergeCoincidentVertBones = true;
    UPROPERTY()
    float VolumeImportance = 1.0f;
    UPROPERTY()
    bool bLockEdges = false;
    UPROPERTY()
    bool bLockColorBounaries = false;
    UPROPERTY()
    bool bImproveTrianglesForCloth = true;
    UPROPERTY()
    int32 BaseLOD = 0;
    UPROPERTY()
    float BoneReductionRatio = 100.0f;
    UPROPERTY()
    TArray<FBoneReference> BonesToRemove;
    UPROPERTY()
    TObjectPtr<UAnimSequence> BakePose = nullptr;
};

}
